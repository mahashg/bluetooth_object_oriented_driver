/*
   BNEP implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2001-2002 Inventel Systemes
   Written 2001-2002 by
	Clément Moreau <clement.moreau@inventel.fr>
	David Libault  <david.libault@inventel.fr>

   Copyright (C) 2002 Maxim Krasnyansky <maxk@qualcomm.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*/

#include <c++/begin_include.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/file.h>
#include <linux/etherdevice.h>
#include <asm/unaligned.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "bnep.h"
#include <c++/end_include.h>

#define VERSION "1.3"

//#ifndef BNEP_CORE_H
class BNEP_CORE {
private:
	struct bnep_session* __bnep_get_session(u8 *dst);
	void __bnep_link_session(struct bnep_session *s);
	void __bnep_unlink_session(struct bnep_session *s);
	int bnep_send(struct bnep_session *s, void *data, size_t len);
	int bnep_send_rsp(struct bnep_session *s, u8 ctrl, u16 resp);
	int bnep_ctrl_set_netfilter(struct bnep_session *s, __be16 *data, int len);
	int bnep_ctrl_set_mcfilter(struct bnep_session *s, u8 *data, int len);
	int bnep_rx_control(struct bnep_session *s, void *data, int len);
	int bnep_rx_extension(struct bnep_session *s, struct sk_buff *skb);
	int bnep_rx_frame(struct bnep_session *s, struct sk_buff *skb);
	int bnep_tx_frame(struct bnep_session *s, struct sk_buff *skb);
//	int bnep_session(void *arg);
	struct device *bnep_get_device(struct bnep_session *session);
	void __bnep_copy_ci(struct bnep_conninfo *ci, struct bnep_session *s);	
	
	#ifdef CONFIG_BT_BNEP_PROTO_FILTER
		inline void bnep_set_default_proto_filter(struct bnep_session *s);
	#endif

public:
	int bnep_add_connection(struct bnep_connadd_req *req, struct socket *sock);
	int bnep_del_connection(struct bnep_conndel_req *req);
	int bnep_get_connlist(struct bnep_connlist_req *req);
	int bnep_get_conninfo(struct bnep_conninfo *ci);

	int bnep_session_cls(void *arg);

}bnep_core;

int bnep_add_connection(struct bnep_connadd_req *req, struct socket *sock);
int bnep_del_connection(struct bnep_conndel_req *req);
int bnep_get_connlist(struct bnep_connlist_req *req);
int bnep_get_conninfo(struct bnep_conninfo *ci);
int bnep_session(void *arg);

bool compress_src = true;
bool compress_dst = true;

static u8 __bnep_rx_hlen[] = {
	ETH_HLEN,     /* BNEP_GENERAL */
	0,            /* BNEP_CONTROL */
	2,            /* BNEP_COMPRESSED */
	ETH_ALEN + 2, /* BNEP_COMPRESSED_SRC_ONLY */
	ETH_ALEN + 2  /* BNEP_COMPRESSED_DST_ONLY */
};

static u8 __bnep_tx_types[] = {
	BNEP_GENERAL,
	BNEP_COMPRESSED_SRC_ONLY,
	BNEP_COMPRESSED_DST_ONLY,
	BNEP_COMPRESSED
};

static struct device_type bnep_type = {
	.name	= "bluetooth",
};
//#endif
