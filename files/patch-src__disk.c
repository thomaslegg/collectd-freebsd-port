--- ./src/disk.c.orig	2014-01-26 16:09:14.856391000 +0800
+++ ./src/disk.c	2014-05-14 10:20:52.000000000 +0800
@@ -732,7 +732,7 @@
 
 #elif defined(HAVE_LIBSTATGRAB)
 	sg_disk_io_stats *ds;
-	int disks, counter;
+	unsigned long disks, counter;
 	char name[DATA_MAX_NAME_LEN];
 	
 	if ((ds = sg_get_disk_io_stats(&disks)) == NULL)
