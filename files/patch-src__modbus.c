--- ./src/modbus.c.orig	2013-04-09 09:13:34.000000000 +0800
+++ ./src/modbus.c	2013-08-09 10:53:23.000000000 +0800
@@ -25,6 +25,9 @@
 #include "plugin.h"
 #include "configfile.h"
 
+/* FreeBSD needs this */
+#include <sys/socket.h> 
+
 #include <netdb.h>
 
 #include <modbus/modbus.h>
