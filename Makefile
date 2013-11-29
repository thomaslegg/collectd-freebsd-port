# Created by: Matt Peterson <matt@peterson.org>
# $FreeBSD: net-mgmt/collectd5/Makefile 329249 2013-10-03 23:37:48Z wg $

PORTNAME=	collectd
PORTVERSION=	5.4.0
PORTREVISION=	1
PKGNAMESUFFIX=	5
CATEGORIES=	net-mgmt
MASTER_SITES=	http://collectd.org/files/

MAINTAINER=	ports@bsdserwis.com
COMMENT=	Systems & network statistics collection daemon

USES=		gmake pkgconfig
USE_BZIP2=	yes
GNU_CONFIGURE=	yes
USE_AUTOTOOLS=	aclocal autoconf autoheader automake libltdl libtool
USE_GNOME=	glib20

OPTIONS_DEFINE=		CGI DEBUG GCRYPT VIRT
OPTIONS_GROUP=		INPUT OUTPUT
OPTIONS_GROUP_OUTPUT=	RRDTOOL NOTIFYEMAIL NOTIFYDESKTOP
OPTIONS_GROUP_INPUT=	CURL DBI JSON MEMCACHEC MODBUS MYSQL \
			NUTUPS PGSQL PING PYTHON RABBITMQ REDIS \
			ROUTEROS SNMP STATGRAB TOKYOTYRANT XML XMMS

CGI_DESC=		Install collection.cgi (requires rrdtool)
CURL_DESC=		Enable curl-based plugins (apache, nginx, etc)
DEBUG_DESC=		Enable debugging
DBI_DESC=		Enable dbi plugin
GCRYPT_DESC=		Build with libgcrypt
JSON_DESC=		Enable JSON plugins
MEMCACHEC_DESC=		Enable memcachec plugin
MODBUS_DESC=		Enable modbus plugin
MYSQL_DESC=		Enable mysql-based plugins
NOTIFYEMAIL_DESC=	Enable notifications via email
NOTIFYDESKTOP_DESC=	Enable desktop notifications
NUTUPS_DESC=		Enable nut (ups) plugin
PGSQL_DESC=		Enable postgresql-based plugins
PING_DESC=		Enable ping plugin
PYTHON_DESC=		Enable python-based plugins
RABBITMQ_DESC=		Enable rabbitmq-based plugins
REDIS_DESC=		Enable redis-based plugins
ROUTEROS_DESC=		Enable routeros plugin
RRDTOOL_DESC=		Enable rrdtool plugin (also rrdcached plugin)
SNMP_DESC=		Enable SNMP plugin
STATGRAB_DESC=		Enable statgrab-based plugins (disk, interface, etc)
TOKYOTYRANT_DESC=	Enable tokyotyrant plugin
VIRT_DESC=		Enable libvirt plugin (requires XML)
XML_DESC=		Enable XML plugins
XMMS_DESC=		Enable xmms plugin

USE_RC_SUBR=	collectd collectdmon

USE_LDCONFIG=	yes

CONFLICTS=	collectd-4.[0-9]*

CPPFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib

.include <bsd.port.options.mk>

# NOTE: Plugins without dependencies are defined further down.
CONFIGURE_ARGS=	--localstatedir=/var \
		--disable-all-plugins \
		--disable-getifaddrs \
		--disable-static \
		--without-java \
		--without-libganglia \
		--without-libiptc \
		--without-libjvm \
		--without-libkstat \
		--without-libmodbus \
		--without-libmongoc \
		--without-libnetlink \
		--without-libnetapp \
		--without-libopenipmi \
		--without-libowcapi \
		--without-libperfstat \
		--without-libperl \
		--without-libsensors \
		--without-libvarnish \
		--without-oracle \
		--without-perl-bindings

# NOTE: Plugins without external dependencies
CONFIGURE_ARGS+=	\
		--enable-aggregation \
		--enable-apcups \
		--enable-contextswitch \
		--enable-cpu \
		--enable-csv \
		--enable-df \
		--enable-dns \
		--enable-email \
		--enable-exec \
		--enable-filecount \
		--enable-load \
		--enable-logfile \
		--enable-match_empty_counter \
		--enable-match_hashed \
		--enable-match_regex \
		--enable-match_timediff \
		--enable-match_value \
		--enable-mbmon \
		--enable-memcached \
		--enable-memory \
		--enable-network \
		--enable-ntpd \
		--enable-openvpn \
		--enable-powerdns \
		--enable-pf \
		--enable-processes \
		--enable-statsd \
		--enable-swap \
		--enable-syslog \
		--enable-table \
		--enable-tail \
		--enable-target_notification \
		--enable-target_replace \
		--enable-target_scale \
		--enable-target_set \
		--enable-target_v5upgrade \
		--enable-tcpconns \
		--enable-teamspeak2 \
		--enable-ted \
		--enable-threshold \
		--enable-unixsock \
		--enable-uptime \
		--enable-uuid \
		--enable-write_graphite \
		--enable-zfs_arc

.if ${PORT_OPTIONS:MCGI}
RUN_DEPENDS+=	p5-URI>=0:${PORTSDIR}/net/p5-URI \
		p5-CGI.pm>=0:${PORTSDIR}/www/p5-CGI.pm \
		p5-Data-Dumper>=0:${PORTSDIR}/devel/p5-Data-Dumper \
		p5-HTML-Parser>=0:${PORTSDIR}/www/p5-HTML-Parser
PLIST_SUB+=	CGI=""
.if empty(PORT_OPTIONS:MRRDTOOL)
IGNORE=		CGI requires RRDTOOL. Either select RRDTOOL or deselect CGI
.endif
.else
PLIST_SUB+=	CGI="@comment "
.endif

.if ${PORT_OPTIONS:MCURL}
LIB_DEPENDS+=	curl:${PORTSDIR}/ftp/curl
CONFIGURE_ARGS+=--with-libcurl=${LOCALBASE} \
		--enable-apache \
		--enable-curl \
		--enable-nginx \
		--enable-write_http
PLIST_SUB+=	CURL=""
.else
CONFIGURE_ARGS+=--without-libcurl \
		--disable-apache \
		--disable-curl \
		--disable-nginx \
		--disable-write_http
PLIST_SUB+=	CURL="@comment "
.endif

.if ${PORT_OPTIONS:MCURL} && ${PORT_OPTIONS:MJSON}
CONFIGURE_ARGS+=--enable-curl_json
PLIST_SUB+=	CURL_JSON=""
.else
CONFIGURE_ARGS+=--disable-curl_json
PLIST_SUB+=	CURL_JSON="@comment "
.endif

.if ${PORT_OPTIONS:MCURL} && ${PORT_OPTIONS:MXML}
#CONFIGURE_ARGS+=	\
#		--enable-ascent \
#		--enable-bind \
#		--enable-curl_xml
#PLIST_SUB+=	CURL_XML=""
#.else
CONFIGURE_ARGS+=	\
		--disable-ascent \
		--disable-bind \
		--disable-curl_xml
PLIST_SUB+=	CURL_XML="@comment "
.endif

.if ${PORT_OPTIONS:MDEBUG}
CONFIGURE_ARGS+=--enable-debug
.else
CONFIGURE_ARGS+=--disable-debug
.endif

.if ${PORT_OPTIONS:MDBI}
LIB_DEPENDS+=	dbi:${PORTSDIR}/databases/libdbi
CONFIGURE_ARGS+=--with-libdbi=${LOCALBASE} --enable-dbi
PLIST_SUB+=	DBI=""
.else
CONFIGURE_ARGS+=--without-libdbi --disable-dbi
PLIST_SUB+=	DBI="@comment "
.endif

.if ${PORT_OPTIONS:MGCRYPT}
LIB_DEPENDS+=	gcrypt:${PORTSDIR}/security/libgcrypt
CONFIGURE_ARGS+=--with-libgcrypt=${LOCALBASE}
LDFLAGS+=	-lgcrypt
.else
CONFIGURE_ARGS+=--without-libgcrypt
.endif

.if ${PORT_OPTIONS:MJSON}
LIB_DEPENDS+=	yajl:${PORTSDIR}/devel/yajl
CONFIGURE_ARGS+=--with-libyajl=${LOCALBASE}
.else
CONFIGURE_ARGS+=--without-libyajl
.endif

.if ${PORT_OPTIONS:MMEMCACHEC}
LIB_DEPENDS+=	memcached:${PORTSDIR}/databases/libmemcached
CONFIGURE_ARGS+=--with-libmemcached=${LOCALBASE} --enable-memcachec
PLIST_SUB+=	MEMCACHEC=""
.else
CONFIGURE_ARGS+=--without-libmemcached
PLIST_SUB+=	MEMCACHEC="@comment "
.endif

.if ${PORT_OPTIONS:MMODBUS}
LIB_DEPENDS+=	modbus:${PORTSDIR}/comms/libmodbus
CONFIGURE_ARGS+=--enable-modbus
PLIST_SUB+=	MODBUS=""
.else
PLIST_SUB+=	MODBUS="@comment "
.endif

.if ${PORT_OPTIONS:MMYSQL}
USE_MYSQL=	yes
CONFIGURE_ARGS+=--with-libmysql=${LOCALBASE} --enable-mysql
PLIST_SUB+=	MYSQL=""
.else
CONFIGURE_ARGS+=--without-libmysql --disable-mysql
PLIST_SUB+=	MYSQL="@comment "
.endif

.if ${PORT_OPTIONS:MNOTIFYDESKTOP}
LIB_DEPENDS+=	notify:${PORTSDIR}/devel/libnotify
CONFIGURE_ARGS+=--with-libnotify=${LOCALBASE} \
		--enable-notify_desktop
PLIST_SUB+=	NOTIFYDESKTOP=""
.else
CONFIGURE_ARGS+=--without-libnotify --disable-notify_desktop
PLIST_SUB+=	NOTIFYDESKTOP="@comment "
.endif

.if ${PORT_OPTIONS:MNOTIFYEMAIL}
LIB_DEPENDS+=	esmtp:${PORTSDIR}/mail/libesmtp
CONFIGURE_ARGS+=--with-libesmtp=${LOCALBASE} \
		--enable-notify_email
PLIST_SUB+=	NOTIFYEMAIL=""
.else
CONFIGURE_ARGS+=--without-libesmtp --disable-notify_email
PLIST_SUB+=	NOTIFYEMAIL="@comment "
.endif

.if ${PORT_OPTIONS:MNUTUPS}
LIB_DEPENDS+=	upsclient:${PORTSDIR}/sysutils/nut
CONFIGURE_ARGS+=--with-upsclient=${LOCALBASE} --enable-nut
PLIST_SUB+=	NUTUPS=""
.else
CONFIGURE_ARGS+=--without-libupsclient --disable-nut
PLIST_SUB+=	NUTUPS="@comment "
.endif

.if ${PORT_OPTIONS:MPGSQL}
USE_PGSQL=	yes
CONFIGURE_ARGS+=--with-postgresql=${LOCALBASE} --enable-postgresql --with-libpq
PLIST_SUB+=	PGSQL=""
.else
CONFIGURE_ARGS+=--without-postgresql --disable-postgresql --without-libpq
PLIST_SUB+=	PGSQL="@comment "
.endif

.if ${PORT_OPTIONS:MPING}
LIB_DEPENDS+=	oping:${PORTSDIR}/net/liboping
CONFIGURE_ARGS+=--with-liboping=${LOCALBASE} --enable-ping
PLIST_SUB+=	PING=""
.else
CONFIGURE_ARGS+=--without-liboping --disable-ping
PLIST_SUB+=	PING="@comment "
.endif

.if ${PORT_OPTIONS:MPYTHON}
USE_PYTHON=	yes
CONFIGURE_ARGS+=--with-python=${PYTHON_CMD} --enable-python
PLIST_SUB+=	PYTHON=""
.else
CONFIGURE_ARGS+=--without-python --disable-python
PLIST_SUB+=	PYTHON="@comment "
.endif

.if ${PORT_OPTIONS:MRABBITMQ}
LIB_DEPENDS+=	rabbitmq:${PORTSDIR}/net/rabbitmq-c
CONFIGURE_ARGS+=--with-librabbitmq=${LOCALBASE} \
		--enable-rabbitmq
PLIST_SUB+=	RABBITMQ=""
.else
CONFIGURE_ARGS+=--without-librabbitmq \
		--disable-rabbitmq
PLIST_SUB+=	RABBITMQ="@comment "
.endif

.if ${PORT_OPTIONS:MREDIS}
LIB_DEPENDS+=	hiredis:${PORTSDIR}/databases/hiredis
CONFIGURE_ARGS+=--with-libhiredis=${LOCALBASE} \
		--enable-redis \
		--enable-write_redis
PLIST_SUB+=	REDIS=""
.else
CONFIGURE_ARGS+=--without-libhiredis \
		--disable-redis \
		--disable-write_redis
PLIST_SUB+=	REDIS="@comment "
.endif

.if ${PORT_OPTIONS:MROUTEROS}
LIB_DEPENDS+=	routeros:${PORTSDIR}/net/librouteros
CONFIGURE_ARGS+=--with-librouteros=${LOCALBASE} --enable-routeros
PLIST_SUB+=	ROUTEROS=""
.else
CONFIGURE_ARGS+=--without-librouteros --disable-routeros
PLIST_SUB+=	ROUTEROS="@comment "
.endif

.if ${PORT_OPTIONS:MRRDTOOL}
LIB_DEPENDS+=	rrd:${PORTSDIR}/databases/rrdtool
CONFIGURE_ARGS+=--with-librrd=${LOCALBASE} \
		--enable-rrdcached \
		--enable-rrdtool
PLIST_SUB+=	RRDTOOL=""
.else
CONFIGURE_ARGS+=--without-librrd \
		--disable-rrdcached \
		--disable-rrdtool
PLIST_SUB+=	RRDTOOL="@comment "
.endif

.if ${PORT_OPTIONS:MSTATGRAB}
USES+=		pkgconfig
LIB_DEPENDS+=	statgrab:${PORTSDIR}/devel/libstatgrab
CONFIGURE_ENV+= LIBS="`pkg-config --libs libstatgrab`"
CONFIGURE_ARGS+=--with-libstatgrab=${LOCALBASE} \
		--enable-disk \
		--enable-interface
PLIST_SUB+=	STATGRAB=""
.else
CONFIGURE_ARGS+=--without-libstatgrab
PLIST_SUB+=	STATGRAB="@comment "
.endif

.if ${OSVERSION} >= 900007
CONFIGURE_ARGS+=--enable-users
PLIST_SUB+= USERS=""
.elif ${PORT_OPTIONS:MSTATGRAB}
CONFIGURE_ARGS+=--enable-users
PLIST_SUB+= USERS=""
.else
PLIST_SUB+= USERS="@comment "
.endif

.if ${PORT_OPTIONS:MSNMP}
LIB_DEPENDS+=	netsnmp:${PORTSDIR}/net-mgmt/net-snmp
CONFIGURE_ARGS+=--with-libnetsnmp --enable-snmp
PLIST_SUB+=	SNMP=""
.else
CONFIGURE_ARGS+=--without-libnetsnmp
PLIST_SUB+=	SNMP="@comment "
.endif

.if ${PORT_OPTIONS:MTOKYOTYRANT}
LIB_DEPENDS+=	tokyotyrant:${PORTSDIR}/databases/tokyotyrant
CONFIGURE_ARGS+=--with-libtokyotyrant=${LOCALBASE} --enable-tokyotyrant
PLIST_SUB+=	TOKYOTYRANT=""
.else
CONFIGURE_ARGS+=--without-libtokyotyrant --disable-tokyotyrant
PLIST_SUB+=	TOKYOTYRANT="@comment "
.endif

.if ${PORT_OPTIONS:MVIRT}
.if empty(PORT_OPTIONS:MXML)
IGNORE=		VIRT requires XML. Either select XML or deselect VIRT.
.endif
LIB_DEPENDS+=	virt.1001:${PORTSDIR}/devel/libvirt
CONFIGURE_ARGS+=--enable-libvirt
PLIST_SUB+=	VIRT=""
.else
CONFIGURE_ARGS+=--without-libvirt --disable-libvirt
PLIST_SUB+=	VIRT="@comment "
.endif

.if ${PORT_OPTIONS:MXML}
LIB_DEPENDS+=	xml2:${PORTSDIR}/textproc/libxml2
CONFIGURE_ARGS+=--with-libxml2=${LOCALBASE}
.endif

.if ${PORT_OPTIONS:MXMMS}
LIB_DEPENDS+=	xmms:${PORTSDIR}/multimedia/xmms
CONFIGURE_ARGS+=--with-libxmms=${LOCALBASE} --enable-xmms
CFLAGS+=	`xmms-config --cflags`
PLIST_SUB+=	XMMS=""
.else
CONFIGURE_ARGS+=--without-libxmms --disable-xmms
PLIST_SUB+=	XMMS="@comment "
.endif

AUTOTOOLSFILES=	aclocal.m4

post-patch:
	@${REINPLACE_CMD} -e 's|1.11.1|%%AUTOMAKE_APIVER%%|g' \
			  -e 's|2.67|%%AUTOCONF_VERSION%%|g' \
			  ${WRKSRC}/aclocal.m4
	@${REINPLACE_CMD} \
		-e 's;@prefix@/var/;/var/;' \
		-e 's;/var/lib/;/var/db/;' \
		-e 's;@localstatedir@/lib/;/var/db/;' \
		${WRKSRC}/src/collectd.conf.in
	@${REINPLACE_CMD} -e '/$$[(]mkinstalldirs)/d' ${WRKSRC}/Makefile.in
	@${REINPLACE_CMD} \
		-e 's;/etc/collection\.conf;${WWWDIR}/collection.conf;' \
		${WRKSRC}/contrib/collection.cgi
	@${REINPLACE_CMD} \
		-e 's;/opt/collectd/var/lib;/var/db;' \
		-e 's;/opt/collectd/lib;${PREFIX}/lib;' \
		${WRKSRC}/contrib/collection.conf
	@${REINPLACE_CMD} \
		-e 's;{libdir}/pkgconfig;{prefix}/libdata/pkgconfig;' \
		${WRKSRC}/configure.in

post-install:
	@${MKDIR} ${STAGEDIR}/var/db/collectd
.if ${PORT_OPTIONS:MCGI}
	@${MKDIR} ${STAGEDIR}${WWWDIR}
	${INSTALL_SCRIPT} ${WRKSRC}/contrib/collection.cgi ${STAGEDIR}${WWWDIR}/
	${INSTALL_DATA} ${WRKSRC}/contrib/collection.conf \
		${STAGEDIR}${WWWDIR}/collection.conf.sample
.endif

.include <bsd.port.mk>
