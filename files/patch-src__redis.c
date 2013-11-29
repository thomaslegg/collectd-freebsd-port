--- ./src/redis.c.orig	2013-08-18 18:24:25.085973000 +0800
+++ ./src/redis.c	2013-11-29 18:23:48.000000000 +0800
@@ -26,7 +26,8 @@
 #include "configfile.h"
 
 #include <pthread.h>
-#include <credis.h>
+#include <sys/time.h>
+#include <hiredis/hiredis.h>
 
 #ifndef HOST_NAME_MAX
 # define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
@@ -37,6 +38,7 @@
 #define REDIS_DEF_PORT    6379
 #define REDIS_DEF_TIMEOUT 2000
 #define MAX_REDIS_NODE_NAME 64
+#define MAX_REDIS_VAL_SIZE 256
 
 /* Redis plugin configuration example:
  *
@@ -44,7 +46,8 @@
  *   <Node "mynode">
  *     Host "localhost"
  *     Port "6379"
- *     Timeout 2000
+ *     Timeout 2
+ *     Password "foobar"
  *   </Node>
  * </Plugin>
  */
@@ -57,7 +60,7 @@
   char host[HOST_NAME_MAX];
   char passwd[HOST_NAME_MAX];
   int port;
-  int timeout;
+  struct timeval timeout;
 
   redis_node_t *next;
 };
@@ -106,16 +109,18 @@
   return (0);
 } /* }}} */
 
+
 static int redis_config_node (oconfig_item_t *ci) /* {{{ */
 {
   redis_node_t rn;
   int i;
   int status;
+  int timeout;
 
   memset (&rn, 0, sizeof (rn));
   sstrncpy (rn.host, REDIS_DEF_HOST, sizeof (rn.host));
   rn.port = REDIS_DEF_PORT;
-  rn.timeout = REDIS_DEF_TIMEOUT;
+  rn.timeout.tv_usec = REDIS_DEF_TIMEOUT;
 
   status = cf_util_get_string_buffer (ci, rn.name, sizeof (rn.name));
   if (status != 0)
@@ -137,7 +142,10 @@
       }
     }
     else if (strcasecmp ("Timeout", option->key) == 0)
-      status = cf_util_get_int (option, &rn.timeout);
+    {
+      status = cf_util_get_int (option, &timeout);
+      if (status == 0) rn.timeout.tv_usec = timeout;
+    }
     else if (strcasecmp ("Password", option->key) == 0)
       status = cf_util_get_string_buffer (option, rn.passwd, sizeof (rn.passwd));
     else
@@ -179,39 +187,14 @@
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
@@ -230,8 +213,14 @@
 
 static int redis_init (void) /* {{{ */
 {
-  redis_node_t rn = { "default", REDIS_DEF_HOST, REDIS_DEF_PASSWD,
-    REDIS_DEF_PORT, REDIS_DEF_TIMEOUT, /* next = */ NULL };
+  redis_node_t rn = {
+    .name = "default",
+    .host = REDIS_DEF_HOST,
+    .port = REDIS_DEF_PORT,
+    .timeout.tv_sec = 0,
+    .timeout.tv_usec = REDIS_DEF_TIMEOUT,
+    .next = NULL
+};
 
   if (nodes_head == NULL)
     redis_node_add (&rn);
@@ -239,20 +228,45 @@
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
 
   for (rn = nodes_head; rn != NULL; rn = rn->next)
   {
-    REDIS rh;
-    REDIS_INFO info;
-
-    int status;
+    redisContext *rh;
+    redisReply   *rr;
 
     DEBUG ("redis plugin: querying info from node `%s' (%s:%d).", rn->name, rn->host, rn->port);
 
-    rh = credis_connect (rn->host, rn->port, rn->timeout);
+    rh = redisConnectWithTimeout ((char *)rn->host, rn->port, rn->timeout);
     if (rh == NULL)
     {
       ERROR ("redis plugin: unable to connect to node `%s' (%s:%d).", rn->name, rn->host, rn->port);
@@ -262,56 +276,37 @@
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
-
-    DEBUG ("redis plugin: received info from node `%s': connected_clients = %d; "
-        "connected_slaves = %d; used_memory = %lu; changes_since_last_save = %lld; "
-        "bgsave_in_progress = %d; total_connections_received = %lld; "
-        "total_commands_processed = %lld; uptime_in_seconds = %ld", rn->name,
-        info.connected_clients, info.connected_slaves, info.used_memory,
-        info.changes_since_last_save, info.bgsave_in_progress,
-        info.total_connections_received, info.total_commands_processed,
-        info.uptime_in_seconds);
-
-    redis_submit_g (rn->name, "current_connections", "clients", info.connected_clients);
-    redis_submit_g (rn->name, "current_connections", "slaves", info.connected_slaves);
-    redis_submit_g (rn->name, "memory", "used", info.used_memory);
-    redis_submit_g (rn->name, "volatile_changes", NULL, info.changes_since_last_save);
-    redis_submit_d (rn->name, "total_connections", NULL, info.total_connections_received);
-    redis_submit_d (rn->name, "total_operations", NULL, info.total_commands_processed);
+    redis_handle_info (rn->name, rr->str, "uptime", NULL, "uptime_in_seconds", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "current_connections", "clients", "connected_clients", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "blocked_clients", NULL, "blocked_clients", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "memory", NULL, "used_memory", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "memory_lua", NULL, "used_memory_lua", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "volatile_changes", NULL, "changes_since_last_save", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "total_connections", NULL, "total_connections_received", DS_TYPE_DERIVE);
+    redis_handle_info (rn->name, rr->str, "total_operations", NULL, "total_commands_processed", DS_TYPE_DERIVE);
+    redis_handle_info (rn->name, rr->str, "expired_keys", NULL, "expired_keys", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "pubsub", "channels", "pubsub_channels", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "pubsub", "patterns", "pubsub_patterns", DS_TYPE_GAUGE);
+    redis_handle_info (rn->name, rr->str, "current_connections", "slaves", "connected_slaves", DS_TYPE_GAUGE);
 
-    credis_close (rh);
+    redisFree (rh);
   }
 
   return 0;
