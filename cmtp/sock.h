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
#include <linux/export.h>

#include <linux/types.h>
#include <linux/capability.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <linux/ioctl.h>
#include <linux/file.h>
#include <linux/compat.h>
#include <linux/gfp.h>
#include <linux/uaccess.h>
#include <net/sock.h>

#include <linux/isdn/capilli.h>
#include "cmtp.h"

#include <c++/end_include.h>


class CMTP_SOCK {
public:
	int cmtp_sock_release(struct socket *sock);
	int cmtp_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
	int cmtp_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
	#ifdef CONFIG_COMPAT
		int cmtp_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
	#endif

}cmtp_sock;

int cmtp_sock_release(struct socket *sock);
static int cmtp_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int cmtp_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
#ifdef CONFIG_COMPAT
	static int cmtp_sock_compat_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
#endif
			    
static const struct net_proto_family cmtp_sock_family_ops = {
	.family	= PF_BLUETOOTH,
	.owner	= THIS_MODULE,
	.create	= cmtp_sock_create
};

static const struct proto_ops cmtp_sock_ops = {
	.family		= PF_BLUETOOTH,
	.owner		= THIS_MODULE,
	.release	= cmtp_sock_release,
	.ioctl		= cmtp_sock_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= cmtp_sock_compat_ioctl,
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

static struct proto cmtp_proto = {
	.name		= "CMTP",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct bt_sock)
};

int cmtp_init_sockets(void);
void cmtp_cleanup_sockets(void);
