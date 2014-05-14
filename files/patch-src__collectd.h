--- ./src/collectd.h.orig	2014-01-26 16:09:23.524559000 +0800
+++ ./src/collectd.h	2014-05-14 10:20:52.000000000 +0800
@@ -235,15 +235,15 @@
 #endif
 
 #ifndef LOCALSTATEDIR
-#define LOCALSTATEDIR PREFIX "/var"
+#define LOCALSTATEDIR "/var"
 #endif
 
 #ifndef PKGLOCALSTATEDIR
-#define PKGLOCALSTATEDIR PREFIX "/var/lib/" PACKAGE_NAME
+#define PKGLOCALSTATEDIR PREFIX "/var/db/" PACKAGE_NAME
 #endif
 
 #ifndef PIDFILE
-#define PIDFILE PREFIX "/var/run/" PACKAGE_NAME ".pid"
+#define PIDFILE "/var/run/" PACKAGE_NAME ".pid"
 #endif
 
 #ifndef PLUGINDIR
