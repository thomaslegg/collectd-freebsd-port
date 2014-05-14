--- ./src/collectd.c.orig	2014-01-26 16:09:14.840391000 +0800
+++ ./src/collectd.c	2014-05-14 10:20:52.000000000 +0800
@@ -288,7 +288,7 @@
 #endif
 
 #if HAVE_LIBSTATGRAB
-	if (sg_init ())
+	if (sg_init (0))
 	{
 		ERROR ("sg_init: %s", sg_str_error (sg_get_error ()));
 		return (-1);
