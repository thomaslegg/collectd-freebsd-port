--- ./version-gen.sh.orig	2013-04-10 10:14:25.000000000 +0800
+++ ./version-gen.sh	2013-08-09 10:53:24.000000000 +0800
@@ -1,13 +1,3 @@
-#!/usr/bin/env bash
-
-DEFAULT_VERSION="5.3.0.git"
-
-VERSION="`git describe 2> /dev/null | sed -e 's/^collectd-//'`"
-
-if test -z "$VERSION"; then
-	VERSION="$DEFAULT_VERSION"
-fi
-
-VERSION="`echo \"$VERSION\" | sed -e 's/-/./g'`"
+#!/bin/sh
 
-echo -n "$VERSION"
+echo -n "5.3.0.git"
