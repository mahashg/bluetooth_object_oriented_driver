/*
   RFCOMM implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2002 Maxim Krasnyansky <maxk@qualcomm.com>
   Copyright (C) 2002 Marcel Holtmann <marcel@holtmann.org>

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

/*
 * RFCOMM sockets.
 */

#include <c++/begin_include.h>
#include <linux/export.h>
#include <linux/debugfs.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/l2cap.h>
#include <net/bluetooth/rfcomm.h>
#include <c++/end_include.h>

class RFCOMM_SOCK{
private:
	/* ---- DLC callbacks ----
	 *
	 * called under rfcomm_dlc_lock()
	 */
//	void rfcomm_sk_data_ready(struct rfcomm_dlc *d, struct sk_buff *skb);
//	void rfcomm_sk_state_change(struct rfcomm_dlc *d, int err);

	/* ---- Socket functions ---- */
	struct sock *__rfcomm_get_sock_by_addr(u8 channel, bdaddr_t *src);

	/* Find socket with channel and source bdaddr.
	 * Returns closest match.
	 */
	struct sock *rfcomm_get_sock_by_channel(int state, u8 channel, bdaddr_t *src);
//	void rfcomm_sock_destruct(struct sock *sk);
	void rfcomm_sock_cleanup_listen(struct sock *parent);

	/* Kill socket (only if zapped and orphan)
	 * Must be called on unlocked socket.
	 */
	void rfcomm_sock_kill(struct sock *sk);
	void __rfcomm_sock_close(struct sock *sk);

	/* Close socket.
	 * Must be called on unlocked socket.
	 */
	void rfcomm_sock_close(struct sock *sk);
	void rfcomm_sock_init(struct sock *sk, struct sock *parent);
	struct sock *rfcomm_sock_alloc(struct net *net, struct socket *sock, int proto, gfp_t prio);
	int rfcomm_sock_setsockopt_old(struct socket *sock, int optname, char __user *optval, unsigned int optlen);
	int rfcomm_sock_getsockopt_old(struct socket *sock, int optname, char __user *optval, int __user *optlen);
//	int rfcomm_sock_debugfs_show(struct seq_file *f, void *p);

	
public:
	int rfcomm_sock_debugfs_open(struct inode *inode, struct file *file);
	int rfcomm_connect_ind(struct rfcomm_session *s, u8 channel, struct rfcomm_dlc **d);
	int rfcomm_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
	
	
	int rfcomm_sock_release(struct socket *sock);
	int rfcomm_sock_bind(struct socket *sock, struct sockaddr *addr, int addr_len);
	int rfcomm_sock_connect(struct socket *sock, struct sockaddr *addr, int alen, int flags);
	int rfcomm_sock_listen(struct socket *sock, int backlog);
	int rfcomm_sock_accept(struct socket *sock, struct socket *newsock, int flags);
	int rfcomm_sock_getname(struct socket *sock, struct sockaddr *addr, int *len, int peer);
	int rfcomm_sock_sendmsg(struct kiocb *iocb, struct socket *sock,
					   struct msghdr *msg, size_t len);
	int rfcomm_sock_recvmsg(struct kiocb *iocb, struct socket *sock,
					   struct msghdr *msg, size_t size, int flags);
	int rfcomm_sock_shutdown(struct socket *sock, int how);
	int rfcomm_sock_getsockopt(struct socket *sock, int level, int optname, char __user *optval, int __user *optlen);
	int rfcomm_sock_setsockopt(struct socket *sock, int level, int optname, char __user *optval, unsigned int optlen);
	int rfcomm_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);


        void rfcomm_sk_data_ready_cls(struct rfcomm_dlc *d, struct sk_buff *skb);
        void rfcomm_sk_state_change_cls(struct rfcomm_dlc *d, int err);
	void rfcomm_sock_destruct_cls(struct sock *sk);
	int rfcomm_sock_debugfs_show_cls(struct seq_file *f, void *p);
}rfcomm_sock;


/** PUBLIC METHODS */
/* ---- RFCOMM core layer callbacks ----
 *
 * called under rfcomm_lock()
 */
int rfcomm_connect_ind(struct rfcomm_session *s, u8 channel, struct rfcomm_dlc **d);
// rfcomm sokc debug fs fops
static int rfcomm_sock_debugfs_open(struct inode *inode, struct file *file);
// rfcomm sock family oper
static int rfcomm_sock_create(struct net *net, struct socket *sock, int protocol, int kern);
// rfcomm proto ops
static int rfcomm_sock_release(struct socket *sock);
static int rfcomm_sock_bind(struct socket *sock, struct sockaddr *addr, int addr_len);
static int rfcomm_sock_connect(struct socket *sock, struct sockaddr *addr, int alen, int flags);
static int rfcomm_sock_listen(struct socket *sock, int backlog);
static int rfcomm_sock_accept(struct socket *sock, struct socket *newsock, int flags);
static int rfcomm_sock_getname(struct socket *sock, struct sockaddr *addr, int *len, int peer);
	static int rfcomm_sock_sendmsg(struct kiocb *iocb, struct socket *sock,
					   struct msghdr *msg, size_t len);
static int rfcomm_sock_recvmsg(struct kiocb *iocb, struct socket *sock,
					   struct msghdr *msg, size_t size, int flags);
static int rfcomm_sock_shutdown(struct socket *sock, int how);
static int rfcomm_sock_getsockopt(struct socket *sock, int level, int optname, char __user *optval, int __user *optlen);
static int rfcomm_sock_setsockopt(struct socket *sock, int level, int optname, char __user *optval, unsigned int optlen);
static int rfcomm_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);


void rfcomm_sk_data_ready(struct rfcomm_dlc *d, struct sk_buff *skb);
void rfcomm_sk_state_change(struct rfcomm_dlc *d, int err);
void rfcomm_sock_destruct(struct sock *sk);
int rfcomm_sock_debugfs_show(struct seq_file *f, void *p);

static struct proto rfcomm_proto = {
	.name		= "RFCOMM",
	.owner		= THIS_MODULE,
	.obj_size	= sizeof(struct rfcomm_pinfo)
};

static const struct file_operations rfcomm_sock_debugfs_fops = {
	.open		= rfcomm_sock_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct proto_ops rfcomm_sock_ops = {
	.family		= PF_BLUETOOTH,
	.owner		= THIS_MODULE,
	.release	= rfcomm_sock_release,
	.bind		= rfcomm_sock_bind,
	.connect	= rfcomm_sock_connect,
	.listen		= rfcomm_sock_listen,
	.accept		= rfcomm_sock_accept,
	.getname	= rfcomm_sock_getname,
	.sendmsg	= rfcomm_sock_sendmsg,
	.recvmsg	= rfcomm_sock_recvmsg,
	.shutdown	= rfcomm_sock_shutdown,
	.setsockopt	= rfcomm_sock_setsockopt,
	.getsockopt	= rfcomm_sock_getsockopt,
	.ioctl		= rfcomm_sock_ioctl,
	.poll		= bt_sock_poll,
	.socketpair	= sock_no_socketpair,
	.mmap		= sock_no_mmap
};

static const struct net_proto_family rfcomm_sock_family_ops = {
	.family		= PF_BLUETOOTH,
	.owner		= THIS_MODULE,
	.create		= rfcomm_sock_create
};

