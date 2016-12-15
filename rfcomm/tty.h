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
 * RFCOMM TTY.
 */

#include <c++/begin_include.h>
#include <linux/module.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/rfcomm.h>
#include <c++/end_include.h>

#define RFCOMM_TTY_MAGIC 0x6d02		/* magic number for rfcomm struct */
#define RFCOMM_TTY_PORTS RFCOMM_MAX_DEV	/* whole lotta rfcomm devices */
#define RFCOMM_TTY_MAJOR 216		/* device node major id of the usb/bluetooth.c driver */
#define RFCOMM_TTY_MINOR 0


/* ---- Device IOCTLs ---- */

#define NOCAP_FLAGS ((1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP))

struct rfcomm_dev {
	struct tty_port		port;
	struct list_head	list;

	char			name[12];
	int			id;
	unsigned long		flags;
	int			err;

	bdaddr_t		src;
	bdaddr_t		dst;
	u8			channel;

	uint			modem_status;

	struct rfcomm_dlc	*dlc;
	wait_queue_head_t       wait;

	struct device		*tty_dev;

	atomic_t		wmem_alloc;

	struct sk_buff_head	pending;
};

static LIST_HEAD(rfcomm_dev_list);
static DEFINE_SPINLOCK(rfcomm_dev_lock);

static DEVICE_ATTR(address, S_IRUGO, show_address, NULL);
static DEVICE_ATTR(channel, S_IRUGO, show_channel, NULL);


/*
static void rfcomm_dev_data_ready(struct rfcomm_dlc *dlc, struct sk_buff *skb);
static void rfcomm_dev_state_change(struct rfcomm_dlc *dlc, int err);
static void rfcomm_dev_modem_status(struct rfcomm_dlc *dlc, u8 v24_sig);
*/
/* ---- Device functions ---- */

/*
 * The reason this isn't actually a race, as you no doubt have a little voice
 * screaming at you in your head, is that the refcount should never actually
 * reach zero unless the device has already been taken off the list, in
 * rfcomm_dev_del(). And if that's not true, we'll hit the BUG() in
 * rfcomm_dev_destruct() anyway.
 */

class RFCOMM_TTY {
private:
	struct rfcomm_dev *__rfcomm_dev_get(int id);

	struct rfcomm_dev *rfcomm_dev_get(int id);

	struct device *rfcomm_get_device(struct rfcomm_dev *dev);

	ssize_t show_address(struct device *tty_dev, struct device_attribute *attr, char *buf);

	ssize_t show_channel(struct device *tty_dev, struct device_attribute *attr, char *buf);

	int rfcomm_dev_add(struct rfcomm_dev_req *req, struct rfcomm_dlc *dlc);

	void rfcomm_dev_del(struct rfcomm_dev *dev);

	/* ---- Send buffer ---- */
	inline unsigned int rfcomm_room(struct rfcomm_dlc *dlc);

	void rfcomm_wfree(struct sk_buff *skb);

	void rfcomm_set_owner_w(struct sk_buff *skb, struct rfcomm_dev *dev);

	struct sk_buff *rfcomm_wmalloc(struct rfcomm_dev *dev, unsigned long size, gfp_t priority);

	/* ---- Device IOCTLs ---- */

	int rfcomm_create_dev(struct sock *sk, void __user *arg);

	int rfcomm_release_dev(void __user *arg);

	int rfcomm_get_dev_list(void __user *arg);

	int rfcomm_get_dev_info(void __user *arg);

	int rfcomm_dev_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);

	/* ---- DLC callbacks ---- */
	void rfcomm_dev_data_ready(struct rfcomm_dlc *dlc, struct sk_buff *skb);

	void rfcomm_dev_state_change(struct rfcomm_dlc *dlc, int err);

	void rfcomm_dev_modem_status(struct rfcomm_dlc *dlc, u8 v24_sig);

	/* ---- TTY functions ---- */
	void rfcomm_tty_copy_pending(struct rfcomm_dev *dev);

public:
	void rfcomm_dev_destruct(struct tty_port *port);
	
	int rfcomm_tty_open(struct tty_struct *tty, struct file *filp);

	void rfcomm_tty_close(struct tty_struct *tty, struct file *filp);

	int rfcomm_tty_write(struct tty_struct *tty, const unsigned char *buf, int count);

	int rfcomm_tty_write_room(struct tty_struct *tty);
	
	int rfcomm_tty_chars_in_buffer(struct tty_struct *tty);

	void rfcomm_tty_flush_buffer(struct tty_struct *tty);

	int rfcomm_tty_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg);

	void rfcomm_tty_throttle(struct tty_struct *tty);

	void rfcomm_tty_unthrottle(struct tty_struct *tty);

	void rfcomm_tty_set_termios(struct tty_struct *tty, struct ktermios *old);

	void rfcomm_tty_send_xchar(struct tty_struct *tty, char ch);

	void rfcomm_tty_hangup(struct tty_struct *tty);

	void rfcomm_tty_wait_until_sent(struct tty_struct *tty, int timeout);

	int rfcomm_tty_tiocmget(struct tty_struct *tty);

	int rfcomm_tty_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear);	
}rfcomm_tty;

// tty port oper
static void rfcomm_dev_destruct(struct tty_port *port);

//tty operations
static int rfcomm_tty_open(struct tty_struct *tty, struct file *filp);

static void rfcomm_tty_close(struct tty_struct *tty, struct file *filp);

static int rfcomm_tty_write(struct tty_struct *tty, const unsigned char *buf, int count);

static int rfcomm_tty_write_room(struct tty_struct *tty);

static int rfcomm_tty_chars_in_buffer(struct tty_struct *tty);

static void rfcomm_tty_flush_buffer(struct tty_struct *tty);

static int rfcomm_tty_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg);

static void rfcomm_tty_throttle(struct tty_struct *tty);

static void rfcomm_tty_unthrottle(struct tty_struct *tty);

static void rfcomm_tty_set_termios(struct tty_struct *tty, struct ktermios *old);

static void rfcomm_tty_send_xchar(struct tty_struct *tty, char ch);

static void rfcomm_tty_hangup(struct tty_struct *tty);

static void rfcomm_tty_wait_until_sent(struct tty_struct *tty, int timeout);

static int rfcomm_tty_tiocmget(struct tty_struct *tty);

static int rfcomm_tty_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear);

/* ---- TTY structure ---- */
static const struct tty_operations rfcomm_ops = {
	.open				= rfcomm_tty_open,
	.close				= rfcomm_tty_close,
	.write				= rfcomm_tty_write,
	.write_room			= rfcomm_tty_write_room,
	.chars_in_buffer	= rfcomm_tty_chars_in_buffer,
	.flush_buffer		= rfcomm_tty_flush_buffer,
	.ioctl				= rfcomm_tty_ioctl,
	.throttle			= rfcomm_tty_throttle,
	.unthrottle			= rfcomm_tty_unthrottle,
	.set_termios		= rfcomm_tty_set_termios,
	.send_xchar			= rfcomm_tty_send_xchar,
	.hangup				= rfcomm_tty_hangup,
	.wait_until_sent	= rfcomm_tty_wait_until_sent,
	.tiocmget			= rfcomm_tty_tiocmget,
	.tiocmset			= rfcomm_tty_tiocmset,
};

static const struct tty_port_operations rfcomm_port_ops = {
	.destruct = rfcomm_dev_destruct,
};

