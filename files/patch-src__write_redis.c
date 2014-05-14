--- ./src/write_redis.c.orig	2014-01-26 16:09:14.920393000 +0800
+++ ./src/write_redis.c	2014-05-14 10:20:52.000000000 +0800
@@ -30,7 +30,8 @@
 #include "configfile.h"
 
 #include <pthread.h>
-#include <credis.h>
+#include <sys/time.h>
+#include <hiredis/hiredis.h>
 
 struct wr_node_s
 {
@@ -38,9 +39,9 @@
 
   char *host;
   int port;
-  int timeout;
+  struct timeval timeout;
 
-  REDIS conn;
+  redisContext *conn;
   pthread_mutex_t lock;
 };
 typedef struct wr_node_s wr_node_t;
@@ -59,6 +60,7 @@
   size_t value_size;
   char *value_ptr;
   int status;
+  redisReply   *rr;
   int i;
 
   status = FORMAT_VL (ident, sizeof (ident), vl);
@@ -105,7 +107,7 @@
 
   if (node->conn == NULL)
   {
-    node->conn = credis_connect (node->host, node->port, node->timeout);
+    node->conn = redisConnectWithTimeout ((char *)node->host, node->port, node->timeout);
     if (node->conn == NULL)
     {
       ERROR ("write_redis plugin: Connecting to host \"%s\" (port %i) failed.",
@@ -116,12 +118,15 @@
     }
   }
 
-  /* "credis_zadd" doesn't handle a NULL pointer gracefully, so I'd rather
-   * have a meaningful assertion message than a normal segmentation fault. */
   assert (node->conn != NULL);
-  status = credis_zadd (node->conn, key, (double) vl->time, value);
+  rr = redisCommand (node->conn, "ZADD %b %f %b", key, sizeof (key),
+    (double) vl->time, value, sizeof (value));
+  if (rr==NULL)
+    WARNING("ZADD command error. key:%s", key);
 
-  credis_sadd (node->conn, "collectd/values", ident);
+  rr = redisCommand (node->conn, "SADD collectd/values %b", ident, sizeof(ident));
+  if (rr==NULL)
+    WARNING("SADD command error. ident:%s", ident);
 
   pthread_mutex_unlock (&node->lock);
 
@@ -137,7 +142,7 @@
 
   if (node->conn != NULL)
   {
-    credis_close (node->conn);
+    redisFree (node->conn);
     node->conn = NULL;
   }
 
@@ -148,6 +153,7 @@
 static int wr_config_node (oconfig_item_t *ci) /* {{{ */
 {
   wr_node_t *node;
+  int timeout;
   int status;
   int i;
 
@@ -157,7 +163,8 @@
   memset (node, 0, sizeof (*node));
   node->host = NULL;
   node->port = 0;
-  node->timeout = 1000;
+  node->timeout.tv_sec = 0;
+  node->timeout.tv_usec = 1000;
   node->conn = NULL;
   pthread_mutex_init (&node->lock, /* attr = */ NULL);
 
@@ -183,8 +190,10 @@
         status = 0;
       }
     }
-    else if (strcasecmp ("Timeout", child->key) == 0)
-      status = cf_util_get_int (child, &node->timeout);
+    else if (strcasecmp ("Timeout", child->key) == 0) {
+      status = cf_util_get_int (child, &timeout);
+      if (status == 0) node->timeout.tv_usec = timeout;
+    }
     else
       WARNING ("write_redis plugin: Ignoring unknown config option \"%s\".",
           child->key);
