--- ./src/redis.c.orig	2013-04-09 09:13:34.000000000 +0800
+++ ./src/redis.c	2013-08-09 11:29:56.000000000 +0800
@@ -26,7 +26,8 @@
 #include "configfile.h"
 
 #include <pthread.h>
-#include <credis.h>
+#include <sys/time.h>
+#include <hiredis/hiredis.h>
 
 #ifndef HOST_NAME_MAX
 # define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
@@ -37,6 +38,10 @@
 #define REDIS_DEF_PORT    6379
 #define REDIS_DEF_TIMEOUT 2000
 #define MAX_REDIS_NODE_NAME 64
+#define MAX_REDIS_QUERY_NAME 64
+#define MAX_REDIS_COMMAND_LEN 256
+#define MAX_REDIS_TYPE_LEN 256
+#define MAX_REDIS_VAL_SIZE 256
 
 /* Redis plugin configuration example:
  *
@@ -44,11 +49,27 @@
  *   <Node "mynode">
  *     Host "localhost"
  *     Port "6379"
- *     Timeout 2000
+ *     Timeout 2
+ *     Password "foobar"
+ *     <Query "len-queue">
+ *        Command "LLEN queue"
+ *        Type "gauge"
+ *     </Query>
  *   </Node>
  * </Plugin>
  */
 
+struct redis_query_s;
+typedef struct redis_query_s redis_query_t;
+struct redis_query_s
+{
+  char name[MAX_REDIS_QUERY_NAME];
+  char command[MAX_REDIS_COMMAND_LEN];
+  char type[MAX_REDIS_TYPE_LEN];
+
+  redis_query_t *next;
+};
+
 struct redis_node_s;
 typedef struct redis_node_s redis_node_t;
 struct redis_node_s
@@ -58,6 +79,7 @@
   char passwd[HOST_NAME_MAX];
   int port;
   int timeout;
+  redis_query_t *query;
 
   redis_node_t *next;
 };
@@ -106,6 +128,72 @@
   return (0);
 } /* }}} */
 
+static int redis_config_query_add (redis_node_t *rn, oconfig_item_t *ci) /* {{{ */
+{
+  redis_query_t *n;
+  int status;
+  int i;
+
+  if ((n=malloc(sizeof(redis_query_t))) == NULL)
+  {
+    return (-1);
+  }
+
+  memset (n, 0, sizeof (redis_query_t));
+
+  if ((status = cf_util_get_string_buffer (ci, n->name, sizeof (n->name))) != 0)
+  {
+    sfree(n);
+    return (-1);
+  }
+
+  for (i = 0; i < ci->children_num; i++)
+  {
+    oconfig_item_t *option = ci->children + i;
+    if (strcasecmp ("Command", option->key) == 0)
+    {
+      if ((option->values_num != 1) || (option->values[0].type != OCONFIG_TYPE_STRING))
+      {
+        WARNING ("redis plugin: `Command' needs exactly one string argument.");
+        continue;
+      }
+      status = cf_util_get_string_buffer (option, n->command, sizeof (n->command));
+    }
+    else if (strcasecmp ("Type", option->key) == 0)
+    {
+      if ((option->values_num != 1) || (option->values[0].type != OCONFIG_TYPE_STRING))
+      {
+        WARNING ("redis plugin: `Type' needs exactly one string argument.");
+        continue;
+      }
+      status = cf_util_get_string_buffer (option, n->type, sizeof (n->type));
+    }
+    else
+      WARNING ("redis plugin: Option `%s' not allowed inside a `Query' "
+          "block. I'll ignore this option.", option->key);
+
+    if (status != 0)
+      break;
+  }
+
+  if (status != 0)
+    return (status);
+
+  if (rn->query == NULL)
+  {
+    rn->query = n;
+  }
+  else
+  {
+    redis_query_t *p;
+    for(p=rn->query; p->next != NULL; p=p->next);
+    p->next = n;
+  }
+
+  return (status);
+
+} /* }}} */
+
 static int redis_config_node (oconfig_item_t *ci) /* {{{ */
 {
   redis_node_t rn;
@@ -140,6 +228,10 @@
       status = cf_util_get_int (option, &rn.timeout);
     else if (strcasecmp ("Password", option->key) == 0)
       status = cf_util_get_string_buffer (option, rn.passwd, sizeof (rn.passwd));
+    else if (strcasecmp ("Query", option->key) == 0)
+    {
+      status = redis_config_query_add(&rn, option);
+    }
     else
       WARNING ("redis plugin: Option `%s' not allowed inside a `Node' "
           "block. I'll ignore this option.", option->key);
@@ -179,39 +271,14 @@
 } /* }}} */
 
   __attribute__ ((nonnull(2)))
-static void redis_submit_g (char *plugin_instance,
-    const char *type, const char *type_instance,
-    gauge_t value) /* {{{ */
-{
-  value_t values[1];
-  value_list_t vl = VALUE_LIST_INIT;
-
-  values[0].gauge = value;
-
-  vl.values = values;
-  vl.values_len = 1;
-  sstrncpy (vl.host, hostname_g, sizeof (vl.host));
-  sstrncpy (vl.plugin, "redis", sizeof (vl.plugin));
-  if (plugin_instance != NULL)
-    sstrncpy (vl.plugin_instance, plugin_instance,
-        sizeof (vl.plugin_instance));
-  sstrncpy (vl.type, type, sizeof (vl.type));
-  if (type_instance != NULL)
-    sstrncpy (vl.type_instance, type_instance,
-        sizeof (vl.type_instance));
-
-  plugin_dispatch_values (&vl);
-} /* }}} */
-
-  __attribute__ ((nonnull(2)))
-static void redis_submit_d (char *plugin_instance,
+static void redis_submit (char *plugin_instance,
     const char *type, const char *type_instance,
-    derive_t value) /* {{{ */
+    value_t value) /* {{{ */
 {
   value_t values[1];
   value_list_t vl = VALUE_LIST_INIT;
 
-  values[0].derive = value;
+  values[0] = value;
 
   vl.values = values;
   vl.values_len = 1;
@@ -230,8 +297,13 @@
 
 static int redis_init (void) /* {{{ */
 {
-  redis_node_t rn = { "default", REDIS_DEF_HOST, REDIS_DEF_PASSWD,
-    REDIS_DEF_PORT, REDIS_DEF_TIMEOUT, /* next = */ NULL };
+  redis_node_t rn = {
+    .name = "default",
+    .host = REDIS_DEF_HOST,
+    .port = REDIS_DEF_PORT,
+    .timeout = REDIS_DEF_TIMEOUT,
+    .next = NULL
+};
 
   if (nodes_head == NULL)
     redis_node_add (&rn);
@@ -239,20 +311,56 @@
   return (0);
 } /* }}} int redis_init */
 
+int redis_handle_info (char *node, char const *info_line, char const *type, char const *type_instance, char const *field_name, int ds_type) /* {{{ */
+{
+  char *str = strstr (info_line, field_name);
+  static char buf[MAX_REDIS_VAL_SIZE];
+  value_t val;
+  if (str)
+  {
+    int i;
+
+    str += strlen (field_name) + 1; /* also skip the ':' */
+    for(i=0;(*str && (isdigit(*str) || *str == '.'));i++,str++)
+      buf[i] = *str;
+    buf[i] ='\0';
+
+    if(parse_value (buf, &val, ds_type) == -1)
+    {
+      WARNING ("redis plugin: Unable to parse field `%s'.", field_name);
+      return (-1);
+    }
+
+    redis_submit (node, type, type_instance, val);
+    return (0);
+  }
+  return (-1);
+
+} /* }}} int redis_handle_info */
+
 static int redis_read (void) /* {{{ */
 {
   redis_node_t *rn;
+  redis_query_t *query;
+
+  const data_set_t *ds;
+  value_list_t vl = VALUE_LIST_INIT;
+  value_t val;
+
 
   for (rn = nodes_head; rn != NULL; rn = rn->next)
   {
-    REDIS rh;
-    REDIS_INFO info;
+    redisContext *rh;
+    redisReply   *rr;
 
-    int status;
+    struct timeval tmout;
+
+    tmout.tv_sec = rn->timeout;
+    tmout.tv_usec = 0;
 
     DEBUG ("redis plugin: querying info from node `%s' (%s:%d).", rn->name, rn->host, rn->port);
 
-    rh = credis_connect (rn->host, rn->port, rn->timeout);
+    rh = redisConnectWithTimeout ((char *)rn->host, rn->port, tmout);
     if (rh == NULL)
     {
       ERROR ("redis plugin: unable to connect to node `%s' (%s:%d).", rn->name, rn->host, rn->port);
@@ -262,56 +370,94 @@
     if (strlen (rn->passwd) > 0)
     {
       DEBUG ("redis plugin: authenticanting node `%s' passwd(%s).", rn->name, rn->passwd);
-      status = credis_auth(rh, rn->passwd);
-      if (status != 0)
+      rr = redisCommand (rh, "AUTH %s", rn->passwd);
+
+      if (rr == NULL || rr->type != 5)
       {
         WARNING ("redis plugin: unable to authenticate on node `%s'.", rn->name);
-        credis_close (rh);
+        redisFree (rh);
         continue;
       }
     }
 
-    memset (&info, 0, sizeof (info));
-    status = credis_info (rh, &info);
-    if (status != 0)
+    if ((rr = redisCommand(rh, "INFO")) == NULL)
     {
-      WARNING ("redis plugin: unable to get info from node `%s'.", rn->name);
-      credis_close (rh);
+      WARNING ("redis plugin: unable to connect to node `%s'.", rn->name);
+      redisFree (rh);
       continue;
     }
 
-    /* typedef struct _cr_info {
-     *   char redis_version[CREDIS_VERSION_STRING_SIZE];
-     *   int bgsave_in_progress;
-     *   int connected_clients;
-     *   int connected_slaves;
-     *   unsigned int used_memory;
-     *   long long changes_since_last_save;
-     *   int last_save_time;
-     *   long long total_connections_received;
-     *   long long total_commands_processed;
-     *   int uptime_in_seconds;
-     *   int uptime_in_days;
-     *   int role;
-     * } REDIS_INFO; */
+    redis_handle_info (rn->name, rr->str, "uptime", NULL, "uptime_in_seconds", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "connections", "clients", "connected_clients", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "connections", "slaves", "connected_slaves", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "blocked_clients", NULL, "blocked_clients", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "memory", NULL, "used_memory", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "changes_since_last_save", NULL, "changes_since_last_save", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "operations", NULL, "total_commands_processed", DS_TYPE_DERIVE);
+    redis_handle_info (rn->name, rr->str, "expired_keys", NULL, "expired_keys", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "pubsub", "patterns", "pubsub_patterns", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "pubsub", "channels", "pubsub_channels", DS_TYPE_GAUGE);
 
-    DEBUG ("redis plugin: received info from node `%s': connected_clients = %d; "
-        "connected_slaves = %d; used_memory = %lu; changes_since_last_save = %lld; "
-        "bgsave_in_progress = %d; total_connections_received = %lld; "
-        "total_commands_processed = %lld; uptime_in_seconds = %ld", rn->name,
-        info.connected_clients, info.connected_slaves, info.used_memory,
-        info.changes_since_last_save, info.bgsave_in_progress,
-        info.total_connections_received, info.total_commands_processed,
-        info.uptime_in_seconds);
+    /* Read custom queries */
+    for (query=rn->query; query!=NULL; query=query->next)
+    {
+      if ((rr = redisCommand (rh, query->command)) == NULL)
+      {
+        WARNING ("redis plugin: unable to execute query `%s' on node `%s'.", query->name, rn->name);
+        continue;
+      }
 
-    redis_submit_g (rn->name, "current_connections", "clients", info.connected_clients);
-    redis_submit_g (rn->name, "current_connections", "slaves", info.connected_slaves);
-    redis_submit_g (rn->name, "memory", "used", info.used_memory);
-    redis_submit_g (rn->name, "volatile_changes", NULL, info.changes_since_last_save);
-    redis_submit_d (rn->name, "total_connections", NULL, info.total_connections_received);
-    redis_submit_d (rn->name, "total_operations", NULL, info.total_commands_processed);
+      if(rr->type != REDIS_REPLY_INTEGER)
+      {
+        WARNING ("redis plugin: unable to get reply for query `%s' on node `%s', integer expected.", query->name, rn->name);
+        continue;
+      }
+      DEBUG("Get data from query `%s' executing `%s' on node `%s'.", query->name, query->command, rn->name);
 
-    credis_close (rh);
+
+      ds = plugin_get_ds (query->type);
+      if (!ds)
+      {
+        ERROR ("redis plugin: DataSet `%s' not defined.", query->type);
+        continue;
+      }
+
+      if (ds->ds_num != 1)
+      {
+        ERROR ("redis plugin: DataSet `%s' requires %i values, but config talks about %i",
+            query->type, ds->ds_num, 1);
+        continue;
+      }
+
+      vl.values_len = ds->ds_num;
+      vl.values = &val;
+
+      if (ds->ds[0].type == DS_TYPE_COUNTER)
+        vl.values[0].counter = rr->integer;
+      else if (ds->ds[0].type == DS_TYPE_DERIVE)
+        vl.values[0].derive = rr->integer;
+      else if (ds->ds[0].type == DS_TYPE_ABSOLUTE)
+        vl.values[0].absolute = rr->integer;
+      else if (ds->ds[0].type == DS_TYPE_GAUGE)
+        vl.values[0].gauge = rr->integer;
+      else
+      {
+        ERROR ("redis plugin: Unknown type `%i' for datasource.", ds->ds[0].type);
+        continue;
+      }
+
+      sstrncpy (vl.host, hostname_g, sizeof (vl.host));
+      sstrncpy (vl.plugin, "redis", sizeof (vl.plugin));
+      sstrncpy (vl.plugin_instance, rn->name, sizeof (vl.plugin));
+      strncat (vl.plugin_instance, "-query", sizeof (vl.plugin));
+      sstrncpy (vl.type, query->type, sizeof (vl.type));
+      sstrncpy (vl.type_instance, query->name, sizeof (vl.type));
+
+      plugin_dispatch_values(&vl);
+
+    }
+
+    redisFree (rh);
   }
 
   return 0;
