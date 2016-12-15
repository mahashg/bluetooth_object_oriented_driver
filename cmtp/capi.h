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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <linux/ioctl.h>
#include <linux/file.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <net/sock.h>

#include <linux/isdn/capilli.h>
#include <linux/isdn/capicmd.h>
#include <linux/isdn/capiutil.h>

#include "cmtp.h"
#include <c++/end_include.h>

#define CAPI_INTEROPERABILITY		0x20

#define CAPI_INTEROPERABILITY_REQ	CAPICMD(CAPI_INTEROPERABILITY, CAPI_REQ)
#define CAPI_INTEROPERABILITY_CONF	CAPICMD(CAPI_INTEROPERABILITY, CAPI_CONF)
#define CAPI_INTEROPERABILITY_IND	CAPICMD(CAPI_INTEROPERABILITY, CAPI_IND)
#define CAPI_INTEROPERABILITY_RESP	CAPICMD(CAPI_INTEROPERABILITY, CAPI_RESP)

#define CAPI_INTEROPERABILITY_REQ_LEN	(CAPI_MSG_BASELEN + 2)
#define CAPI_INTEROPERABILITY_CONF_LEN	(CAPI_MSG_BASELEN + 4)
#define CAPI_INTEROPERABILITY_IND_LEN	(CAPI_MSG_BASELEN + 2)
#define CAPI_INTEROPERABILITY_RESP_LEN	(CAPI_MSG_BASELEN + 2)

#define CAPI_FUNCTION_REGISTER		0
#define CAPI_FUNCTION_RELEASE		1
#define CAPI_FUNCTION_GET_PROFILE	2
#define CAPI_FUNCTION_GET_MANUFACTURER	3
#define CAPI_FUNCTION_GET_VERSION	4
#define CAPI_FUNCTION_GET_SERIAL_NUMBER	5
#define CAPI_FUNCTION_MANUFACTURER	6
#define CAPI_FUNCTION_LOOPBACK		7


#define CMTP_MSGNUM	1
#define CMTP_APPLID	2
#define CMTP_MAPPING	3

class CMTP_CAPI {
	private:
		struct cmtp_application *cmtp_application_add(struct cmtp_session *session, __u16 appl);
		void cmtp_application_del(struct cmtp_session *session, struct cmtp_application *app);
		struct cmtp_application *cmtp_application_get(struct cmtp_session *session, int pattern, __u16 value);

		int cmtp_msgnum_get(struct cmtp_session *session);
		void cmtp_send_capimsg(struct cmtp_session *session, struct sk_buff *skb);
		void cmtp_send_interopmsg(struct cmtp_session *session,
							__u8 subcmd, __u16 appl, __u16 msgnum,
							__u16 function, unsigned char *buf, int len);
		void cmtp_recv_interopmsg(struct cmtp_session *session, struct sk_buff *skb);
		
		int cmtp_load_firmware(struct capi_ctr *ctrl, capiloaddata *data);
		void cmtp_reset_ctr(struct capi_ctr *ctrl);
		void cmtp_register_appl(struct capi_ctr *ctrl, __u16 appl, capi_register_params *rp);
		void cmtp_release_appl(struct capi_ctr *ctrl, __u16 appl);
		u16 cmtp_send_message(struct capi_ctr *ctrl, struct sk_buff *skb);
		char *cmtp_procinfo(struct capi_ctr *ctrl);
		int cmtp_proc_show(struct seq_file *m, void *v);

	public:
		void cmtp_recv_capimsg(struct cmtp_session *session, struct sk_buff *skb);
		int cmtp_attach_device(struct cmtp_session *session);
		void cmtp_detach_device(struct cmtp_session *session);
		int cmtp_proc_open(struct inode *inode, struct file *file);

} cmtp_capi;

void cmtp_recv_capimsg(struct cmtp_session *session, struct sk_buff *skb);
int cmtp_attach_device(struct cmtp_session *session);
void cmtp_detach_device(struct cmtp_session *session);	
static int cmtp_proc_open(struct inode *inode, struct file *file);
	
static const struct file_operations cmtp_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= cmtp_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

