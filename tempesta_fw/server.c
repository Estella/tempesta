/**
 *		Tempesta FW
 *
 * Servers handling.
 *
 * Copyright (C) 2012-2014 NatSys Lab. (info@natsys-lab.com).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <linux/slab.h>

#include "connection.h"
#include "addr.h"
#include "lib.h"
#include "log.h"
#include "sched.h"
#include "server.h"

static struct kmem_cache *srv_cache;

static void
validate_srv(const TfwServer *srv)
{
	BUG_ON(!srv);

	IF_DEBUG {
		TfwConnection *conn;
		struct sock *sk;
		int i;

		tfw_ptrset_for_each(sk, i, &srv->conn_pool) {
			conn = sk->sk_user_data;
			BUG_ON(conn->peer != &srv->peer);
			BUG_ON(sk->sk_socket->state == SS_FREE);
		}
	}
}



TfwServer *
tfw_server_create(const TfwAddr *addr)
{
	TfwServer *srv;

	srv =  kmem_cache_alloc(srv_cache, GFP_ATOMIC | __GFP_ZERO);
	if (!srv) {
		TFW_ERR("Can't allocate an ojbect from srv_cache\n");
		return NULL;
	}

	srv->addr = *addr;

	validate_srv(srv);

	return srv;
}

void
tfw_server_destroy(TfwServer *srv)
{
	struct sock *sk;
	int i;

	if (!srv)
		return;

	validate_srv(srv);

	tfw_ptrset_for_each(sk, i, &srv->conn_pool) {
		sock_release(sk->sk_socket);
	}

	/* FIXME: clear the server references from all current sessions.
	 * That should be done only for forward proxy.
	 * For reverse proxy this is done implicitly since all backend servers
	 * are allocated only once upon init and then freed on exit at a point
	 * when all sessions are already terminated.
	 */
	kmem_cache_free(srv_cache, srv);
}

struct sock *
tfw_server_get_conn(TfwServer *srv)
{
	return tfw_ptrset_get_rr(&srv->conn_pool);
}

int
tfw_server_snprint(const TfwServer *srv, char *buf, size_t buf_size)
{
	char addr_str_buf[MAX_ADDR_LEN];

	BUG_ON(!srv || !buf || !buf_size);


	tfw_inet_ntop(&srv->addr, addr_str_buf);

	return snprintf(buf, buf_size, "srv %p: %s", srv, addr_str_buf);
}
EXPORT_SYMBOL(tfw_server_snprint);

int __init
tfw_server_init(void)
{
	srv_cache = kmem_cache_create("tfw_srv_cache", sizeof(TfwServer),
				       0, 0, NULL);
	if (!srv_cache)
		return -ENOMEM;
	return 0;
}

void
tfw_server_exit(void)
{
	kmem_cache_destroy(srv_cache);
}

