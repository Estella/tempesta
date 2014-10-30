/**
 *		Tempesta FW
 *
 * Definitions for generic connection (at OSI level 4) management.
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
#ifndef __TFW_CONNECTION_H__
#define __TFW_CONNECTION_H__

#include "gfsm.h"
#include "msg.h"
#include "session.h"

typedef enum {
	/* Protocol bits. */
	__Conn_Bits	= 0x8,

	/* Each connection has Client or Server bit. */
	Conn_Clnt	= 0x1 << __Conn_Bits,
	Conn_Srv	= 0x2 << __Conn_Bits,

	Conn_HttpClnt	= Conn_Clnt | TFW_FSM_HTTP,
	Conn_HttpSrv	= Conn_Srv | TFW_FSM_HTTP,
} tfw_conn_type_t;

#define TFW_CONN_TYPE2IDX(t)	TFW_FSM_TYPE(t)

typedef enum {
	TFW_CONN_CLOSE_FREE,
	TFW_CONN_CLOSE_LEAVE
} tfw_conn_close_action_t;

typedef struct TfwConnection TfwConnection;
typedef struct TfwPeer TfwPeer;
typedef struct TfwPeerOps TfwPeerOps;

typedef tfw_conn_close_action_t (*tfw_conn_close_cb_t)(TfwConnection *conn);

/* TODO backend connection could have many sessions. */
struct TfwConnection {
	/*
	 * Stack of l5-l7 protocol handlers.
	 * Base class, must be first.
	 */
	SsProto		proto;

	struct sock	*sk;	/* Connection is a wrapper for this socket. */
	TfwMsg		*msg;	/* Currently processing (receiving) message. */
	TfwSession	*sess;

	TfwPeer *peer;			  /* TfwClient, TfwServer, etc. */
	struct list_head peer_conn_list;  /* A peer has many connections. */

	/* The callback decides what to do when a connection is closed by the
	 * remote side: deallocate or leave as is (for further reconnection). */
	tfw_conn_close_cb_t close_cb;
};

#define TFW_CONN_TYPE(c)	((c)->proto.type)

/* Connection downcalls. */
int tfw_connection_new(struct sock *sk, tfw_conn_type_t type, TfwPeer *peer,
		       tfw_conn_close_cb_t disconn_cb);
void tfw_connection_send_cli(TfwSession *sess, TfwMsg *msg);
int tfw_connection_send_srv(TfwSession *sess, TfwMsg *msg);


/* Callbacks used by l5-l7 protocols to operate on connection level. */
typedef struct {
	/*
	 * Before servicing a new connection (client or server - connection
	 * type should be checked in the callback).
	 * This is a good place to handle Access or GEO modules (block a client
	 * or bind its descriptor with Geo information).
	 */
	int (*conn_init)(TfwConnection *conn);

	/*
	 * Closing a connection (client or server as for conn_init()).
	 * This is necessary for modules who account number of established
	 * client connections.
	 */
	void (*conn_destruct)(TfwConnection *conn);

	/**
	 * High level protocols should be able to allocate messages with all
	 * required information.
	 */
	TfwMsg * (*conn_msg_alloc)(TfwConnection *conn);
} TfwConnHooks;

void tfw_connection_hooks_register(TfwConnHooks *hooks, tfw_conn_type_t type);


struct TfwPeer {
	const TfwPeerOps *ops;
};

struct TfwPeerOps {
	int (*peer_attach_conn)(TfwPeer *peer, TfwConnection *conn);
	void (*peer_detach_conn)(TfwPeer *peer, TfwConnection *conn);
	void (*peer_destroy)(TfwPeer *peer);
};

static inline int
tfw_peer_attach_conn(TfwPeer *peer, TfwConnection *conn)
{
	BUG_ON(!peer || !conn || !peer->ops->peer_attach_conn);
	return peer->ops->peer_attach_conn(peer, conn);
}

static inline void
tfw_peer_detach_conn(TfwPeer *peer, TfwConnection *conn)
{
	BUG_ON(!peer || !conn || !peer->ops->peer_detach_conn);
	peer->ops->peer_detach_conn(peer, conn);
}

static inline void
tfw_peer_destroy(TfwPeer *peer)
{
	if (!peer)
		return;
	BUG_ON(!peer->ops->peer_destroy);
	peer->ops->peer_destroy(peer);
}

#endif /* __TFW_CONNECTION_H__ */
