--- ./version-gen.sh.orig	2014-01-26 16:09:23.540560000 +0800
+++ ./version-gen.sh	2014-05-14 10:20:52.000000000 +0800
@@ -1,13 +1,3 @@
-#!/usr/bin/env bash
-
-DEFAULT_VERSION="5.4.1.git"
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
+echo -n "5.4.1.git"
