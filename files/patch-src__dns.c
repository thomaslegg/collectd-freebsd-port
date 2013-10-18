--- ./src/dns.c.orig	2013-04-09 09:13:34.000000000 +0800
+++ ./src/dns.c	2013-08-09 10:53:23.000000000 +0800
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
