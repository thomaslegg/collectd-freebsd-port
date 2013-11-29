--- ./src/write_graphite.c.orig	2013-08-18 18:24:25.093973000 +0800
+++ ./src/write_graphite.c	2013-11-29 18:42:18.000000000 +0800
@@ -35,8 +35,6 @@
   *   <Carbon>
   *     Host "localhost"
   *     Port "2003"
-  *     Protocol "udp"
-  *     LogSendErrors true
   *     Prefix "collectd"
   *   </Carbon>
   * </Plugin>
@@ -66,14 +64,6 @@
 # define WG_DEFAULT_SERVICE "2003"
 #endif
 
-#ifndef WG_DEFAULT_PROTOCOL
-# define WG_DEFAULT_PROTOCOL "udp"
-#endif
-
-#ifndef WG_DEFAULT_LOG_SEND_ERRORS
-# define WG_DEFAULT_LOG_SEND_ERRORS 1
-#endif
-
 #ifndef WG_DEFAULT_ESCAPE
 # define WG_DEFAULT_ESCAPE '_'
 #endif
@@ -94,8 +84,6 @@
 
     char    *node;
     char    *service;
-    char    *protocol;
-    _Bool   log_send_errors;
     char    *prefix;
     char    *postfix;
     char     escape_char;
@@ -128,11 +116,10 @@
     ssize_t status = 0;
 
     status = swrite (cb->sock_fd, cb->send_buf, strlen (cb->send_buf));
-    if (cb->log_send_errors && status < 0)
+    if (status < 0)
     {
         char errbuf[1024];
-        ERROR ("write_graphite plugin: send to %s:%s (%s) failed with status %zi (%s)",
-                cb->node, cb->service, cb->protocol,
+        ERROR ("write_graphite plugin: send failed with status %zi (%s)",
                 status, sstrerror (errno, errbuf, sizeof (errbuf)));
 
 
@@ -186,7 +173,6 @@
 
     const char *node = cb->node ? cb->node : WG_DEFAULT_NODE;
     const char *service = cb->service ? cb->service : WG_DEFAULT_SERVICE;
-    const char *protocol = cb->protocol ? cb->protocol : WG_DEFAULT_PROTOCOL;
 
     if (cb->sock_fd > 0)
         return (0);
@@ -196,19 +182,15 @@
     ai_hints.ai_flags |= AI_ADDRCONFIG;
 #endif
     ai_hints.ai_family = AF_UNSPEC;
-
-    if (0 == strcasecmp ("tcp", protocol))
-        ai_hints.ai_socktype = SOCK_STREAM;
-    else
-        ai_hints.ai_socktype = SOCK_DGRAM;
+    ai_hints.ai_socktype = SOCK_STREAM;
 
     ai_list = NULL;
 
     status = getaddrinfo (node, service, &ai_hints, &ai_list);
     if (status != 0)
     {
-        ERROR ("write_graphite plugin: getaddrinfo (%s, %s, %s) failed: %s",
-                node, service, protocol, gai_strerror (status));
+        ERROR ("write_graphite plugin: getaddrinfo (%s, %s) failed: %s",
+                node, service, gai_strerror (status));
         return (-1);
     }
 
@@ -237,16 +219,17 @@
     {
         char errbuf[1024];
         c_complain (LOG_ERR, &cb->init_complaint,
-                "write_graphite plugin: Connecting to %s:%s via %s failed. "
-                "The last error was: %s", node, service, protocol,
+                "write_graphite plugin: Connecting to %s:%s failed. "
+                "The last error was: %s", node, service,
                 sstrerror (errno, errbuf, sizeof (errbuf)));
+        close (cb->sock_fd);
         return (-1);
     }
     else
     {
         c_release (LOG_INFO, &cb->init_complaint,
-                "write_graphite plugin: Successfully connected to %s:%s via %s.",
-                node, service, protocol);
+                "write_graphite plugin: Successfully connected to %s:%s.",
+                node, service);
     }
 
     wg_reset_buffer (cb);
@@ -267,15 +250,11 @@
 
     wg_flush_nolock (/* timeout = */ 0, cb);
 
-    if (cb->sock_fd >= 0)
-    {
-        close (cb->sock_fd);
-        cb->sock_fd = -1;
-    }
+    close(cb->sock_fd);
+    cb->sock_fd = -1;
 
     sfree(cb->name);
     sfree(cb->node);
-    sfree(cb->protocol);
     sfree(cb->service);
     sfree(cb->prefix);
     sfree(cb->postfix);
@@ -356,10 +335,9 @@
     cb->send_buf_fill += message_len;
     cb->send_buf_free -= message_len;
 
-    DEBUG ("write_graphite plugin: [%s]:%s (%s) buf %zu/%zu (%.1f %%) \"%s\"",
+    DEBUG ("write_graphite plugin: [%s]:%s buf %zu/%zu (%.1f %%) \"%s\"",
             cb->node,
             cb->service,
-            cb->protocol,
             cb->send_buf_fill, sizeof (cb->send_buf),
             100.0 * ((double) cb->send_buf_fill) / ((double) sizeof (cb->send_buf)),
             message);
@@ -389,9 +367,12 @@
         return (status);
 
     /* Send the message to graphite */
-    status = wg_send_message (buffer, cb);
-    if (status != 0) /* error message has been printed already. */
+    wg_send_message (buffer, cb);
+    if (status != 0)
+    {
+        /* An error message has already been printed. */
         return (status);
+    }
 
     return (0);
 } /* int wg_write_messages */
@@ -449,7 +430,6 @@
     user_data_t user_data;
     char callback_name[DATA_MAX_NAME_LEN];
     int i;
-    int status = 0;
 
     cb = malloc (sizeof (*cb));
     if (cb == NULL)
@@ -462,8 +442,6 @@
     cb->name = NULL;
     cb->node = NULL;
     cb->service = NULL;
-    cb->protocol = NULL;
-    cb->log_send_errors = WG_DEFAULT_LOG_SEND_ERRORS;
     cb->prefix = NULL;
     cb->postfix = NULL;
     cb->escape_char = WG_DEFAULT_ESCAPE;
@@ -472,7 +450,7 @@
     /* FIXME: Legacy configuration syntax. */
     if (strcasecmp ("Carbon", ci->key) != 0)
     {
-        status = cf_util_get_string (ci, &cb->name);
+        int status = cf_util_get_string (ci, &cb->name);
         if (status != 0)
         {
             wg_callback_free (cb);
@@ -491,20 +469,6 @@
             cf_util_get_string (child, &cb->node);
         else if (strcasecmp ("Port", child->key) == 0)
             cf_util_get_service (child, &cb->service);
-        else if (strcasecmp ("Protocol", child->key) == 0)
-        {
-            cf_util_get_string (child, &cb->protocol);
-
-            if (strcasecmp ("UDP", cb->protocol) != 0 &&
-                strcasecmp ("TCP", cb->protocol) != 0)
-            {
-                ERROR ("write_graphite plugin: Unknown protocol (%s)",
-                        cb->protocol);
-                status = -1;
-            }
-        }
-        else if (strcasecmp ("LogSendErrors", child->key) == 0)
-            cf_util_get_boolean (child, &cb->log_send_errors);
         else if (strcasecmp ("Prefix", child->key) == 0)
             cf_util_get_string (child, &cb->prefix);
         else if (strcasecmp ("Postfix", child->key) == 0)
@@ -524,25 +488,14 @@
         {
             ERROR ("write_graphite plugin: Invalid configuration "
                         "option: %s.", child->key);
-            status = -1;
         }
-
-        if (status != 0)
-            break;
-    }
-
-    if (status != 0)
-    {
-        wg_callback_free (cb);
-        return (status);
     }
 
     /* FIXME: Legacy configuration syntax. */
     if (cb->name == NULL)
-        ssnprintf (callback_name, sizeof (callback_name), "write_graphite/%s/%s/%s",
+        ssnprintf (callback_name, sizeof (callback_name), "write_graphite/%s/%s",
                 cb->node != NULL ? cb->node : WG_DEFAULT_NODE,
-                cb->service != NULL ? cb->service : WG_DEFAULT_SERVICE,
-                cb->protocol != NULL ? cb->protocol : WG_DEFAULT_PROTOCOL);
+                cb->service != NULL ? cb->service : WG_DEFAULT_SERVICE);
     else
         ssnprintf (callback_name, sizeof (callback_name), "write_graphite/%s",
                 cb->name);
