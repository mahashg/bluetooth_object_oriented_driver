/*
   BNEP implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2001-2002 Inventel Systemes
   Written 2001-2002 by
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
#include <linux/export.h>
#include <linux/file.h>

#include "bnep.h"
#include <c++/end_include.h>

class BNEP_SOCK {
public:
	int bnep_sock_release(struct socket *sock);
	int bnep_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
	int bnep_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
	
	#ifdef CONFIG_COMPAT
		int bnep_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
	#endif
} bnep_sock;

static int bnep_sock_release(struct socket *sock);
static int bnep_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int bnep_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
	
#ifdef CONFIG_COMPAT
	static int bnep_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
#endif


static const struct net_proto_family bnep_sock_family_ops = {
	.family = PF_BLUETOOTH,
	.owner	= THIS_MODULE,
	.create = bnep_sock_create
};

static const struct proto_ops bnep_sock_ops = {
	.family		= PF_BLUETOOTH,
	.owner		= THIS_MODULE,
	.release	= bnep_sock_release,
	.ioctl		= bnep_sock_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= bnep_sock_compat_ioctl,
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

static struct proto bnep_proto = {
	.name		= "BNEP",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct bt_sock)
};

int __init bnep_sock_init(void);
void __exit bnep_sock_cleanup(void);
