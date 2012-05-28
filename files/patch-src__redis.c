--- src/redis.c.orig	2012-04-02 16:04:58.000000000 +0800
+++ src/redis.c	2012-05-06 12:38:33.000000000 +0800
@@ -26,6 +26,9 @@
 #include "configfile.h"
 
 #include <pthread.h>
+#if HAVE_NETDB_H
+# include <netdb.h> /* NI_MAXHOST */
+#endif
 #include <credis.h>
 
 #define REDIS_DEF_HOST   "localhost"
@@ -49,7 +52,8 @@
 struct redis_node_s
 {
   char name[MAX_REDIS_NODE_NAME];
-  char host[HOST_NAME_MAX];
+  /*  char host[HOST_NAME_MAX]; */
+  char host[NI_MAXHOST];
   int port;
   int timeout;
 
