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
 * Bluetooth RFCOMM core.
 */

#include <c++/begin_include.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <asm/unaligned.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/l2cap.h>
#include <net/bluetooth/rfcomm.h>

#include "fcs_computation.h"

#include <c++/end_include.h>

#define VERSION "1.11"

#define rfcomm_lock()	mutex_lock(&rfcomm_mutex)
#define rfcomm_unlock()	mutex_unlock(&rfcomm_mutex)

static bool disable_cfc;
static bool l2cap_ertm;
static int channel_mtu = -1;
static unsigned int l2cap_mtu = RFCOMM_MAX_L2CAP_MTU;

static struct task_struct *rfcomm_thread;

static DEFINE_MUTEX(rfcomm_mutex);
static LIST_HEAD(session_list);

static void rfcomm_schedule(void)
{
	if (!rfcomm_thread)
		return;
	wake_up_process(rfcomm_thread);
}

/** ABOVE CODE IS ONLY FOR COMPUTATION PURPOSE AND HENCE NOT TO BE PUT IN MAIN CLASS**/

class RFCOMM_CORE{
private:
	/* ---- L2CAP callbacks ---- */

	int rfcomm_l2sock_create(struct socket **sock);

	int rfcomm_check_security(struct rfcomm_dlc *d);

	void rfcomm_session_set_timer(struct rfcomm_session *s, long timeout);

	void rfcomm_session_clear_timer(struct rfcomm_session *s);

	/* ---- RFCOMM DLCs ---- */
	void rfcomm_dlc_set_timer(struct rfcomm_dlc *d, long timeout);

	void rfcomm_dlc_clear_timer(struct rfcomm_dlc *d);

	void rfcomm_dlc_clear_state(struct rfcomm_dlc *d);

//	struct rfcomm_dlc *rfcomm_dlc_alloc(gfp_t prio);

	void rfcomm_dlc_link(struct rfcomm_session *s, struct rfcomm_dlc *d);

	void rfcomm_dlc_unlink(struct rfcomm_dlc *d);

	struct rfcomm_dlc *rfcomm_dlc_get(struct rfcomm_session *s, u8 dlci);

	int __rfcomm_dlc_open(struct rfcomm_dlc *d, bdaddr_t *src, bdaddr_t *dst, u8 channel);

	int __rfcomm_dlc_close(struct rfcomm_dlc *d, int err);

	/* ---- RFCOMM sessions ---- */
	struct rfcomm_session *rfcomm_session_add(struct socket *sock, int state);

	struct rfcomm_session *rfcomm_session_del(struct rfcomm_session *s);

	struct rfcomm_session *rfcomm_session_get(bdaddr_t *src, bdaddr_t *dst);

	struct rfcomm_session *rfcomm_session_close(struct rfcomm_session *s,
							   int err);

	struct rfcomm_session *rfcomm_session_create(bdaddr_t *src,
								bdaddr_t *dst,
								u8 sec_level,
								int *err);

	/* ---- RFCOMM frame sending ---- */
	int rfcomm_send_frame(struct rfcomm_session *s, u8 *data, int len);

	int rfcomm_send_cmd(struct rfcomm_session *s, struct rfcomm_cmd *cmd);

	int rfcomm_send_sabm(struct rfcomm_session *s, u8 dlci);

	int rfcomm_send_ua(struct rfcomm_session *s, u8 dlci);

	int rfcomm_send_disc(struct rfcomm_session *s, u8 dlci);

	int rfcomm_queue_disc(struct rfcomm_dlc *d);

	int rfcomm_send_dm(struct rfcomm_session *s, u8 dlci);

	int rfcomm_send_nsc(struct rfcomm_session *s, int cr, u8 type);

	int rfcomm_send_pn(struct rfcomm_session *s, int cr, struct rfcomm_dlc *d);

	int rfcomm_send_rls(struct rfcomm_session *s, int cr, u8 dlci, u8 status);

	int rfcomm_send_msc(struct rfcomm_session *s, int cr, u8 dlci, u8 v24_sig);

	int rfcomm_send_fcoff(struct rfcomm_session *s, int cr);

	int rfcomm_send_fcon(struct rfcomm_session *s, int cr);

	int rfcomm_send_test(struct rfcomm_session *s, int cr, u8 *pattern, int len);

	int rfcomm_send_credits(struct rfcomm_session *s, u8 addr, u8 credits);

	void rfcomm_make_uih(struct sk_buff *skb, u8 addr);

	/* ---- RFCOMM frame reception ---- */
	struct rfcomm_session *rfcomm_recv_ua(struct rfcomm_session *s, u8 dlci);

	struct rfcomm_session *rfcomm_recv_dm(struct rfcomm_session *s, u8 dlci);

	struct rfcomm_session *rfcomm_recv_disc(struct rfcomm_session *s,
							   u8 dlci);

	void rfcomm_check_accept(struct rfcomm_dlc *d);

	int rfcomm_recv_sabm(struct rfcomm_session *s, u8 dlci);

	int rfcomm_apply_pn(struct rfcomm_dlc *d, int cr, struct rfcomm_pn *pn);

	int rfcomm_recv_pn(struct rfcomm_session *s, int cr, struct sk_buff *skb);

	int rfcomm_recv_rpn(struct rfcomm_session *s, int cr, int len, struct sk_buff *skb);

	int rfcomm_recv_rls(struct rfcomm_session *s, int cr, struct sk_buff *skb);

	int rfcomm_recv_msc(struct rfcomm_session *s, int cr, struct sk_buff *skb);

	int rfcomm_recv_mcc(struct rfcomm_session *s, struct sk_buff *skb);

	int rfcomm_recv_data(struct rfcomm_session *s, u8 dlci, int pf, struct sk_buff *skb);

	struct rfcomm_session *rfcomm_recv_frame(struct rfcomm_session *s,
							struct sk_buff *skb);

	/* ---- Connection and data processing ---- */
	void rfcomm_process_connect(struct rfcomm_session *s);

	/* Send data queued for the DLC.
	 * Return number of frames left in the queue.
	 */
	int rfcomm_process_tx(struct rfcomm_dlc *d);

	void rfcomm_process_dlcs(struct rfcomm_session *s);

	struct rfcomm_session *rfcomm_process_rx(struct rfcomm_session *s);

	void rfcomm_accept_connection(struct rfcomm_session *s);

	struct rfcomm_session *rfcomm_check_connection(struct rfcomm_session *s);

	void rfcomm_process_sessions(void);

	int rfcomm_add_listener(bdaddr_t *ba);

	void rfcomm_kill_listener(void);

//	int rfcomm_run(void *unused);

//	int rfcomm_dlc_debugfs_show(struct seq_file *f, void *x);

public:
/* public methods */
	void rfcomm_dlc_free(struct rfcomm_dlc *d);

	int rfcomm_dlc_open(struct rfcomm_dlc *d, bdaddr_t *src, bdaddr_t *dst, u8 channel);

	int rfcomm_dlc_close(struct rfcomm_dlc *d, int err);

	int rfcomm_dlc_send(struct rfcomm_dlc *d, struct sk_buff *skb);

	void __rfcomm_dlc_throttle(struct rfcomm_dlc *d);

	void __rfcomm_dlc_unthrottle(struct rfcomm_dlc *d);

	int rfcomm_dlc_set_modem_status(struct rfcomm_dlc *d, u8 v24_sig);

	int rfcomm_dlc_get_modem_status(struct rfcomm_dlc *d, u8 *v24_sig);

	void rfcomm_session_getaddr(struct rfcomm_session *s, bdaddr_t *src, bdaddr_t *dst);

	void rfcomm_dlc_accept(struct rfcomm_dlc *d);

	int rfcomm_send_rpn(struct rfcomm_session *s, int cr, u8 dlci,
				u8 bit_rate, u8 data_bits, u8 stop_bits,
				u8 parity, u8 flow_ctrl_settings,
				u8 xon_char, u8 xoff_char, u16 param_mask);

	/* exposed because they are used in others */				
	// hci_cb
	void rfcomm_security_cfm_cls(struct hci_conn *conn, u8 status, u8 encrypt);
	// file operations
	int rfcomm_dlc_debugfs_open_cls(struct inode *inode, struct file *file);

	 // method access related
	void rfcomm_l2state_change_cls(struct sock *sk);
	void rfcomm_l2data_ready_cls(struct sock *sk, int bytes);
	void rfcomm_dlc_timeout_cls(unsigned long arg);
	void rfcomm_session_timeout_cls(unsigned long arg);
	int rfcomm_run_cls(void *unused);
        struct rfcomm_dlc *rfcomm_dlc_alloc_cls(gfp_t prio);
	int rfcomm_dlc_debugfs_show_cls(struct seq_file *f, void *x);
}rfcomm_core;


void rfcomm_dlc_free(struct rfcomm_dlc *d);

int rfcomm_dlc_open(struct rfcomm_dlc *d, bdaddr_t *src, bdaddr_t *dst, u8 channel);

int rfcomm_dlc_close(struct rfcomm_dlc *d, int err);

int rfcomm_dlc_send(struct rfcomm_dlc *d, struct sk_buff *skb);

void __rfcomm_dlc_throttle(struct rfcomm_dlc *d);

void __rfcomm_dlc_unthrottle(struct rfcomm_dlc *d);

int rfcomm_dlc_set_modem_status(struct rfcomm_dlc *d, u8 v24_sig);

int rfcomm_dlc_get_modem_status(struct rfcomm_dlc *d, u8 *v24_sig);

void rfcomm_session_getaddr(struct rfcomm_session *s, bdaddr_t *src, bdaddr_t *dst);

void rfcomm_dlc_accept(struct rfcomm_dlc *d);

int rfcomm_send_rpn(struct rfcomm_session *s, int cr, u8 dlci,
				u8 bit_rate, u8 data_bits, u8 stop_bits,
				u8 parity, u8 flow_ctrl_settings,
				u8 xon_char, u8 xoff_char, u16 param_mask);

/* exposed because they are used in others */	
// hci_cb
static void rfcomm_security_cfm(struct hci_conn *conn, u8 status, u8 encrypt);
// file operations
static int rfcomm_dlc_debugfs_open(struct inode *inode, struct file *file);



// exposed
void rfcomm_l2state_change(struct sock *sk);
void rfcomm_l2data_ready(struct sock *sk, int bytes);
void rfcomm_dlc_timeout(unsigned long arg);
void rfcomm_session_timeout(unsigned long arg);
int rfcomm_run(void *unused);
struct rfcomm_dlc *rfcomm_dlc_alloc(gfp_t prio);
int rfcomm_dlc_debugfs_show(struct seq_file *f, void *x);

static struct hci_cb rfcomm_cb = {
	.name		= "RFCOMM",
	.security_cfm	= rfcomm_security_cfm
};

static const struct file_operations rfcomm_dlc_debugfs_fops = {
	.open		= rfcomm_dlc_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
