--- ./src/dns.c.orig	2014-01-26 16:09:14.856391000 +0800
+++ ./src/dns.c	2014-05-14 10:20:52.000000000 +0800
@@ -223,6 +223,15 @@
 		pthread_sigmask (SIG_SETMASK, &sigmask, NULL);
 	}
 
+	/* Check for pcap_device, if needed */
+#ifdef __sparc64__
+	if (pcap_device == NULL)
+	{
+		ERROR ("dns plugin: Interface required");
+		return (NULL);
+	}
+#endif
+
 	/* Passing `pcap_device == NULL' is okay and the same as passign "any" */
 	DEBUG ("dns plugin: Creating PCAP object..");
 	pcap_obj = pcap_open_live ((pcap_device != NULL) ? pcap_device : "any",
