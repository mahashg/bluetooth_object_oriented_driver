/*
   HIDP implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2003-2004 Marcel Holtmann <marcel@holtmann.org>

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
#include <linux/export.h>
#include <linux/file.h>

#include "hidp.h"
#include <c++/end_include.h>

#ifdef CONFIG_COMPAT
struct compat_hidp_connadd_req {
	int   ctrl_sock;	/* Connected control socket */
	int   intr_sock;	/* Connected interrupt socket */
	__u16 parser;
	__u16 rd_size;
	compat_uptr_t rd_data;
	__u8  country;
	__u8  subclass;
	__u16 vendor;
	__u16 product;
	__u16 version;
	__u32 flags;
	__u32 idle_to;
	char  name[128];
};

#endif

class HIDP_SOCK{
public:
	int hidp_sock_release(struct socket *sock);
	int hidp_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
	int hidp_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

	#ifdef CONFIG_COMPAT
		int hidp_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
	#endif

} hidp_sock;

static int hidp_sock_release(struct socket *sock);
static int hidp_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
static int hidp_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

#ifdef CONFIG_COMPAT
	static int hidp_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
#endif


static const struct proto_ops hidp_sock_ops = {
	.family		= PF_BLUETOOTH,
	.owner		= THIS_MODULE,
	.release	= hidp_sock_release,
	.ioctl		= hidp_sock_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= hidp_sock_compat_ioctl,
#endif
	.bind		= sock_no_bind,
	.getname	= sock_no_getname,
	.sendmsg	= sock_no_sendmsg,
	.recvmsg	= sock_no_recvmsg,
	.poll		= sock_no_poll,
	.listen		= sock_no_listen,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_no_setsockopt,
	.getsockopt	= sock_no_getsockopt,
	.connect	= sock_no_connect,
	.socketpair	= sock_no_socketpair,
	.accept		= sock_no_accept,
	.mmap		= sock_no_mmap
};

static struct proto hidp_proto = {
	.name		= "HIDP",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct bt_sock)
};

static const struct net_proto_family hidp_sock_family_ops = {
	.family	= PF_BLUETOOTH,
	.owner	= THIS_MODULE,
	.create	= hidp_sock_create
};

