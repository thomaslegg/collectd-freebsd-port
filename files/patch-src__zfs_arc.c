--- ./src/zfs_arc.c.orig	2013-04-08 23:23:16.000000000 +0800
+++ ./src/zfs_arc.c	2013-04-28 16:13:09.000000000 +0800
@@ -19,6 +19,8 @@
  * Authors:
  *   Anthony Dewhurst <dewhurst at gmail>
  *   Aurelien Rougemont <beorn at gandi.net>
+ *   Brad Davis <brd at FreeBSD.org>
+ *   William Grzybowski <william88 at gmail>
  **/
 
 #include "collectd.h"
@@ -29,7 +31,14 @@
  * Global variables
  */
 
+#if defined(HAVE_LIBKSTAT)
+static kstat_t *ksp;
 extern kstat_ctl_t *kc;
+#elif defined(HAVE_SYSCTLBYNAME)
+  typedef void kstat_t;
+# include <sys/types.h>
+# include <sys/sysctl.h>
+#endif /* HAVE_SYSCTLBYNAME */
 
 static void za_submit (const char* type, const char* type_instance, value_t* values, int values_len)
 {
@@ -57,37 +66,56 @@
 static int za_read_derive (kstat_t *ksp, const char *kstat_value,
     const char *type, const char *type_instance)
 {
-  long long tmp;
-  value_t v;
+	long long tmp;
+	value_t v;
 
-  tmp = get_kstat_value (ksp, (char *)kstat_value);
-  if (tmp == -1LL)
-  {
-    ERROR ("zfs_arc plugin: Reading kstat value \"%s\" failed.", kstat_value);
-    return (-1);
-  }
+#if defined(HAVE_LIBKSTAT)
+	tmp = get_kstat_value (ksp, (char *)kstat_value);
+	if (tmp == -1LL)
+	{
+		ERROR ("zfs_arc plugin: Reading kstat value \"%s\" failed.", kstat_value);
+		return (-1);
+	}
+#elif defined(HAVE_SYSCTLBYNAME)
+	size_t size;
+	size = sizeof(tmp);
+	if (sysctlbyname(kstat_value, &tmp, &size, NULL, 0) < 0) {
+		ERROR ("zfs_arc plugin: Reading sysctl \"%s\" failed.", kstat_value);
+		return (-1);
+	}
+#endif /* HAVE_LIBKSTAT */
 
-  v.derive = (derive_t) tmp;
-  za_submit (type, type_instance, /* values = */ &v, /* values_num = */ 1);
-  return (0);
+	v.derive = (derive_t) tmp;
+	za_submit (type, type_instance, /* values = */ &v, /* values_num = */ 1);
+	return (0);
 }
 
 static int za_read_gauge (kstat_t *ksp, const char *kstat_value,
     const char *type, const char *type_instance)
 {
-  long long tmp;
-  value_t v;
+	long long tmp;
+	value_t v;
 
-  tmp = get_kstat_value (ksp, (char *)kstat_value);
-  if (tmp == -1LL)
-  {
-    ERROR ("zfs_arc plugin: Reading kstat value \"%s\" failed.", kstat_value);
-    return (-1);
-  }
+#if defined(HAVE_LIBKSTAT)
+	tmp = get_kstat_value (ksp, (char *)kstat_value);
+	if (tmp == -1LL)
+	{
+		ERROR ("zfs_arc plugin: Reading kstat value \"%s\" failed.", kstat_value);
+		return (-1);
+	}
 
-  v.gauge = (gauge_t) tmp;
-  za_submit (type, type_instance, /* values = */ &v, /* values_num = */ 1);
-  return (0);
+#elif defined(HAVE_SYSCTLBYNAME)
+	size_t size;
+	size = sizeof(tmp);
+	if (sysctlbyname(kstat_value, &tmp, &size, NULL, 0) < 0) {
+		ERROR ("zfs_arc plugin: Reading sysctl \"%s\" failed.", kstat_value);
+		return (-1);
+	}
+#endif /* HAVE_LIBKSTAT */
+
+	v.gauge = (gauge_t) tmp;
+	za_submit (type, type_instance, /* values = */ &v, /* values_num = */ 1);
+	return (0);
 }
 
 static void za_submit_ratio (const char* type_instance, gauge_t hits, gauge_t misses)
@@ -105,11 +133,67 @@
 	za_submit_gauge ("cache_ratio", type_instance, ratio);
 }
 
-static int za_read (void)
+#if defined(HAVE_SYSCTLBYNAME)
+static int za_read_freebsd (void)
+{
+	// Sizes
+	za_read_gauge (NULL, "kstat.zfs.misc.arcstats.size", "cache_size", "arc");
+	za_read_gauge (NULL, "kstat.zfs.misc.arcstats.l2_size", "cache_size", "L2");
+
+	/* Operations */
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.allocated", "cache_operation", "allocated");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.deleted",   "cache_operation", "deleted");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.stolen",    "cache_operation", "stolen");
+
+	/* Issue indicators */
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.mutex_miss",      "mutex_operation", "miss");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.hash_collisions", "hash_collisions", "");
+
+	/* Evictions */
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.evict_l2_cached",     "cache_eviction", "cached");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.evict_l2_eligible",   "cache_eviction", "eligible");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.evict_l2_ineligible", "cache_eviction", "ineligible");
+
+	/* Hits / misses */
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.demand_data_hits",         "cache_result", "demand_data-hit");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.demand_metadata_hits",     "cache_result", "demand_metadata-hit");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.prefetch_data_hits",       "cache_result", "prefetch_data-hit");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.prefetch_metadata_hits",   "cache_result", "prefetch_metadata-hit");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.demand_data_misses",       "cache_result", "demand_data-miss");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.demand_metadata_misses",   "cache_result", "demand_metadata-miss");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.prefetch_data_misses",     "cache_result", "prefetch_data-miss");
+	za_read_derive (NULL, "kstat.zfs.misc.arcstats.prefetch_metadata_misses", "cache_result", "prefetch_metadata-miss");
+
+	/* Ratios */
+	gauge_t arc_hits;
+	gauge_t arc_misses;
+	gauge_t l2_hits;
+	gauge_t l2_misses;
+	value_t  l2_io[2];
+
+	size_t size;
+	size = sizeof(arc_hits);
+	sysctlbyname("kstat.zfs.misc.arcstats.hits", &arc_hits, &size, NULL, 0);
+	sysctlbyname("kstat.zfs.misc.arcstats.misses", &arc_misses, &size, NULL, 0);
+	sysctlbyname("kstat.zfs.misc.arcstats.l2_hits", &l2_hits, &size, NULL, 0);
+	sysctlbyname("kstat.zfs.misc.arcstats.l2_misses", &l2_misses, &size, NULL, 0);
+
+	za_submit_ratio ("arc", arc_hits, arc_misses);
+	za_submit_ratio ("L2", l2_hits, l2_misses);
+
+	/* I/O */
+	sysctlbyname("kstat.zfs.misc.arcstats.l2_read_bytes", &l2_io[0].derive, &size, NULL, 0);
+	sysctlbyname("kstat.zfs.misc.arcstats.l2_write_bytes", &l2_io[1].derive, &size, NULL, 0);
+
+	za_submit ("io_octets", "L2", l2_io, /* num values = */ 2);
+
+	return (0);
+}
+#elif defined(HAVE_LIBKSTAT)
+static int za_read_solaris (void)
 {
 	gauge_t  arc_hits, arc_misses, l2_hits, l2_misses;
 	value_t  l2_io[2];
-	kstat_t	 *ksp	= NULL;
 
 	get_kstat (&ksp, "zfs", 0, "arcstats");
 	if (ksp == NULL)
@@ -163,15 +247,20 @@
 
 	return (0);
 } /* int za_read */
+#endif
 
 static int za_init (void) /* {{{ */
 {
+#if defined(HAVE_LIBKSTAT)
+	ksp = NULL;
+
 	/* kstats chain already opened by update_kstat (using *kc), verify everything went fine. */
 	if (kc == NULL)
 	{
 		ERROR ("zfs_arc plugin: kstat chain control structure not available.");
 		return (-1);
 	}
+#endif /* HAVE_LIBKSTAT */
 
 	return (0);
 } /* }}} int za_init */
@@ -179,7 +268,16 @@
 void module_register (void)
 {
 	plugin_register_init ("zfs_arc", za_init);
-	plugin_register_read ("zfs_arc", za_read);
+
+#if defined(HAVE_LIBKSTAT)
+	plugin_register_read ("zfs_arc", za_read_solaris);
+#elif defined(HAVE_SYSCTLBYNAME)
+	plugin_register_read ("zfs_arc", za_read_freebsd);
+#else
+	ERROR ("Unable to determine which OS we are on");
+	return (-1);
+#endif /* HAVE_SYSCTLBYNAME */
+
 } /* void module_register */
 
 /* vmi: set sw=8 noexpandtab fdm=marker : */
