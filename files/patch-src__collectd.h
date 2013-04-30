--- ./src/collectd.h.orig	2013-04-09 09:13:34.000000000 +0800
+++ ./src/collectd.h	2013-04-28 16:13:09.000000000 +0800
@@ -235,7 +235,7 @@
 #endif
 
 #ifndef LOCALSTATEDIR
-#define LOCALSTATEDIR PREFIX "/var"
+#define LOCALSTATEDIR "/var"
 #endif
 
 #ifndef PKGLOCALSTATEDIR
@@ -243,7 +243,7 @@
 #endif
 
 #ifndef PIDFILE
-#define PIDFILE PREFIX "/var/run/" PACKAGE_NAME ".pid"
+#define PIDFILE "/var/run/" PACKAGE_NAME ".pid"
 #endif
 
 #ifndef PLUGINDIR
