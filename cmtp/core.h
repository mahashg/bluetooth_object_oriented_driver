/*
   CMTP implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2002-2003 Marcel Holtmann <marcel@holtmann.org>

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

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/freezer.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <linux/ioctl.h>
#include <linux/file.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/sock.h>

#include <linux/isdn/capilli.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/l2cap.h>

#include "cmtp.h"
#include <c++/end_include.h>

#define VERSION "1.0"

static DECLARE_RWSEM(cmtp_session_sem);
static LIST_HEAD(cmtp_session_list);

class CMTP_CORE {
private:
	struct cmtp_session *__cmtp_get_session(bdaddr_t *bdaddr);
	void __cmtp_link_session(struct cmtp_session *session);
	void __cmtp_unlink_session(struct cmtp_session *session);
	void __cmtp_copy_session(struct cmtp_session *session, struct cmtp_conninfo *ci);
	inline int cmtp_alloc_block_id(struct cmtp_session *session);
	inline void cmtp_free_block_id(struct cmtp_session *session, int id);
	inline void cmtp_add_msgpart(struct cmtp_session *session, int id, const unsigned char *buf, int count);
	inline int cmtp_recv_frame(struct cmtp_session *session, struct sk_buff *skb);
	int cmtp_send_frame(struct cmtp_session *session, unsigned char *data, int len);
	void cmtp_process_transmit(struct cmtp_session *session);
	int cmtp_session(void *arg);

public:
	int cmtp_add_connection(struct cmtp_connadd_req *req, struct socket *sock);
	int cmtp_del_connection(struct cmtp_conndel_req *req);
	int cmtp_get_connlist(struct cmtp_connlist_req *req);
	int cmtp_get_conninfo(struct cmtp_conninfo *ci);

}cmtp_core;

int cmtp_add_connection(struct cmtp_connadd_req *req, struct socket *sock);
int cmtp_del_connection(struct cmtp_conndel_req *req);
int cmtp_get_connlist(struct cmtp_connlist_req *req);
int cmtp_get_conninfo(struct cmtp_conninfo *ci);
