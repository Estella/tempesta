/**
 *		Tempesta FW
 *
 * Clients handling.
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
#include <linux/kernel.h>
#include <linux/slab.h>

#include "client.h"
#include "log.h"

static struct kmem_cache *cli_cache;

static TfwClient *
get_cli_by_peer(TfwPeer *peer)
{
	BUG_ON(!peer);
	return container_of(peer, TfwClient, peer);
}

static void
validate_cli_conn(const TfwClient *cli, const TfwConnection *conn)
{
	BUG_ON(!conn || !(TFW_CONN_TYPE(conn) & Conn_Clnt));

	/* The client and connection must be either linked with each other
	 * or not linked at all. */
	BUG_ON(!cli->conn ^ !conn->peer);
	if (cli->conn) {
		BUG_ON(cli->conn != conn);
		BUG_ON(conn->peer != &cli->peer);
	}
}

static int
tfw_client_attach_conn(TfwPeer *peer, TfwConnection *conn)
{
	TfwClient *cli = get_cli_by_peer(peer);
	validate_cli_conn(cli, conn);

	TFW_DBG("Attach conn %p to client %p\n", conn, cli);

	cli->conn = conn;
	conn->peer = &cli->peer;

	return 0;
}

static void
tfw_client_detach_conn(TfwPeer *peer, TfwConnection *conn)
{
	TfwClient *cli = get_cli_by_peer(peer);
	validate_cli_conn(cli, conn);

	TFW_DBG("Detach conn %p from client %p\n", conn, cli);

	if (!cli->conn)
		TFW_WARN("The connection is not attached to the client\n");

	conn->peer = NULL;
	cli->conn = NULL;
}

static void
tfw_client_destroy(TfwPeer *peer)
{
	TfwClient *cli = get_cli_by_peer(peer);

	if (unlikely(!cli))
		return;

	TFW_DBG("Destroy client: %p\n", cli);

	BUG_ON(cli->conn);
	kmem_cache_free(cli_cache, cli);
}


static const __read_mostly TfwPeerOps tfw_client_peer_ops = {
	.peer_attach_conn = tfw_client_attach_conn,
	.peer_detach_conn = tfw_client_detach_conn,
	.peer_destroy = tfw_client_destroy
};

TfwClient *
tfw_client_create(void)
{
	TfwClient *cli = kmem_cache_alloc(cli_cache, GFP_ATOMIC | __GFP_ZERO);
	if (unlikely(!cli)) {
		TFW_ERR("Can't allocate an object from cli_cache\n");
		return NULL;
	}

	cli->peer.ops = &tfw_client_peer_ops;

	TFW_DBG("Created client: %p\n", cli);

	return cli;
}

int __init
tfw_client_init(void)
{
	cli_cache = kmem_cache_create("tfw_client_cache", sizeof(TfwClient),
				       0, 0, NULL);
	if (!cli_cache)
		return -ENOMEM;
	return 0;
}

void
tfw_client_exit(void)
{
	kmem_cache_destroy(cli_cache);
}
