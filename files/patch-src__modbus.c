--- ./src/modbus.c.orig	2014-01-26 16:09:14.876392000 +0800
+++ ./src/modbus.c	2014-05-14 10:20:52.000000000 +0800
@@ -25,6 +25,9 @@
 #include "plugin.h"
 #include "configfile.h"
 
+/* FreeBSD needs this */
+#include <sys/socket.h> 
+
 #include <netdb.h>
 
 #include <modbus/modbus.h>
