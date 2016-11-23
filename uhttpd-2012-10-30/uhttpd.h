/*
 * uhttpd - Tiny single-threaded httpd - Main header
 *
 *   Copyright (C) 2010-2012 Jo-Philipp Wich <xm@subsignal.org>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _UHTTPD_

#define _BSD_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <dlfcn.h>

#ifdef REMOTE_MANAGE
#include <pthread.h>
#endif

#include <libubox/list.h>
#include <libubox/uloop.h>

#include <net/if.h>


#ifdef HAVE_LUA
#include <lua.h>
#endif

#ifdef HAVE_TLS
#include <openssl/ssl.h>
#endif

/* uClibc... */
#ifndef SOL_TCP
#define SOL_TCP	6
#endif

#ifdef DEBUG
#define MSG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define MSG_LOG(...)
#define D(...)
#endif

#define UH_LIMIT_MSGHEAD	4096
#define UH_LIMIT_HEADERS	64
#define UH_LIMIT_CLIENTS	64

#define SLP_WAN_DEV_NAME_MAX_LEN	32
#define SLP_WAN_DEV_PROTO_MAX_LEN	16
#define SLP_WAN_LISTEN_PORT_MAX_LEN	8
#define SLP_WAN_LISTEN_MODE_MAX_LEN	4
#define SLP_WAN_LISTEN_HOST_MAX_LEN 128

#define SLP_UHTTPD_PACKAGE_NAME		"uhttpd"
#define SLP_UHTTPD_MAIN_SECT_NAME	"main"
#define SLP_UHTTPD_WAN_PORT_OPT		"listen_http_wan"

#define SLP_NETWORK_PACKAGE_NAME	"network"
#define SLP_NETWORK_WAN_SECT_NAME	"wan"
#define SLP_NETWORK_WAN_PROTO_OPT	"proto"
#define SLP_NETWORK_WAN_IFNAME_OPT	"ifname"

#define SLP_FIREWALL_PACKAGE_NAME	"firewall"
#define SLP_FIREWALL_WAN_MANAGE_SECT_NAME	"wan_manage"
#define SLP_FIREWALL_WAN_MODE_OPT	"mode"
#define SLP_FIREWALL_WAN_FORBID		"0"

#define SLP_WAN_PROTO_STATIC		"static"
#define SLP_WAN_PROTO_DHCP			"dhcp"
#define SLP_WAN_PROTO_PPPOE			"none"
#define SLP_PPPOE_WAN_DEV_NAME		"pppoe-internet"
#define SLP_LAN_DEVICE "br-lan"
#define SLP_WAN_DEVICE "eth0"
//#define SLP_CGI_BIN_LUCI	"/cgi-bin/luci"
#define SLP_CGI_BIN_LUCI	"/cgi-bin"
#define SLP_LUCI_STATIC		"/web-static"

#define SLP_LISTEN_HTTP_WAN "80"
#define SLP_LISTEN_HTTPS    "443"

struct listener;
struct client;
struct interpreter;
struct http_request;
struct uh_ubus_state;

#ifdef REMOTE_MANAGE
typedef struct
{
	char dev_name[SLP_WAN_DEV_NAME_MAX_LEN];
	char wan_proto[SLP_WAN_DEV_PROTO_MAX_LEN];
	char listen_host[SLP_WAN_LISTEN_HOST_MAX_LEN];
	char listen_port[SLP_WAN_LISTEN_PORT_MAX_LEN];
	char listen_mode[SLP_WAN_LISTEN_MODE_MAX_LEN];
} WAN_CONF_INFO;
#endif

struct config {
	char docroot[PATH_MAX];
#ifdef REMOTE_MANAGE
	char old_dev_name[SLP_WAN_DEV_NAME_MAX_LEN];
	char old_wan_port[SLP_WAN_LISTEN_PORT_MAX_LEN];
#endif
	char *realm;
	char *file;
	char *error_handler;
#ifdef REMOTE_MANAGE
	char *wan_dev_name;
	struct listener *wan_listener;
	struct listener *old_wan_listener;
	int wan_listen_success;
#endif
	int no_symlinks;
	int no_dirlists;
	int network_timeout;
	int rfc1918_filter;
	int tcp_keepalive;
	int max_requests;
#ifdef HAVE_CGI
	char *cgi_prefix;
#endif
#ifdef HAVE_LUA
	char *lua_prefix;
	char *lua_handler;
	lua_State *lua_state;
	lua_State * (*lua_init) (const struct config *conf);
	void (*lua_close) (lua_State *L);
	bool (*lua_request) (struct client *cl, lua_State *L);
#endif
#ifdef HAVE_UBUS
	char *ubus_prefix;
	char *ubus_socket;
	void *ubus_state;
	struct uh_ubus_state * (*ubus_init) (const struct config *conf);
	void (*ubus_close) (struct uh_ubus_state *state);
	bool (*ubus_request) (struct client *cl, struct uh_ubus_state *state);
#endif
#if defined(HAVE_CGI) || defined(HAVE_LUA) || defined(HAVE_UBUS)
	int script_timeout;
#endif
#ifdef HAVE_TLS
	char *cert;
	char *key;
	SSL_CTX *tls;
	SSL_CTX * (*tls_init) (void);
	int (*tls_cert) (SSL_CTX *c, const char *file);
	int (*tls_key) (SSL_CTX *c, const char *file);
	void (*tls_free) (struct listener *l);
	int (*tls_accept) (struct client *c);
	void (*tls_close) (struct client *c);
	int (*tls_recv) (struct client *c, char *buf, int len);
	int (*tls_send) (struct client *c, const char *buf, int len);
#endif
};

enum http_method {
	UH_HTTP_MSG_GET,
	UH_HTTP_MSG_POST,
	UH_HTTP_MSG_HEAD,
	UH_HTTP_MSG_PUT,
};

extern const char *http_methods[];

enum http_version {
	UH_HTTP_VER_0_9,
	UH_HTTP_VER_1_0,
	UH_HTTP_VER_1_1,
};

extern const char *http_versions[];

struct http_request {
	enum http_method method;
	enum http_version version;
	int redirect_status;
	char *url;
	char *headers[UH_LIMIT_HEADERS];
	struct auth_realm *realm;
};

struct http_response {
	int statuscode;
	char *statusmsg;
	char *headers[UH_LIMIT_HEADERS];
};

struct listener {
	struct uloop_fd fd;
	int socket;
#ifdef REMOTE_MANAGE
	int work;
#endif
	int n_clients;
	struct sockaddr_in6 addr;
	struct config *conf;
#ifdef HAVE_TLS
	SSL_CTX *tls;
#endif
	struct listener *next;
};

struct client {
#ifdef HAVE_TLS
	SSL *tls;
#endif
	struct uloop_fd fd;
	struct uloop_fd rpipe;
	struct uloop_fd wpipe;
	struct uloop_process proc;
	struct uloop_timeout timeout;
	bool (*cb)(struct client *);
	void *priv;
	bool dispatched;
	struct {
		char buf[UH_LIMIT_MSGHEAD];
		char *ptr;
		int len;
	} httpbuf;
	struct listener *server;
	struct http_request request;
	struct http_response response;
	struct sockaddr_in6 servaddr;
	struct sockaddr_in6 peeraddr;
	struct client *next;
};

struct client_light {
#ifdef HAVE_TLS
	SSL *tls;
#endif
	struct uloop_fd fd;
};

struct auth_realm {
	char path[PATH_MAX];
	char user[32];
	char pass[128];
	struct auth_realm *next;
};

struct index_file {
	const char *name;
	struct index_file *next;
};

#ifdef HAVE_CGI
struct interpreter {
	char path[PATH_MAX];
	char extn[32];
	struct interpreter *next;
};
#endif

#endif
