/*
   HIDP implementation for Linux Bluetooth stack (BlueZ).
   Copyright (C) 2003-2004 Marcel Holtmann <marcel@holtmann.org>
   Copyright (C) 2013 David Herrmann <dh.herrmann@gmail.com>

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
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/hidraw.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/l2cap.h>

#include "hidp.h"
#include <c++/end_include.h>
#define VERSION "1.2"

static DECLARE_RWSEM(hidp_session_sem);
static LIST_HEAD(hidp_session_list);

static unsigned char hidp_keycode[256] = {
	  0,   0,   0,   0,  30,  48,  46,  32,  18,  33,  34,  35,  23,  36,
	 37,  38,  50,  49,  24,  25,  16,  19,  31,  20,  22,  47,  17,  45,
	 21,  44,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  28,   1,
	 14,  15,  57,  12,  13,  26,  27,  43,  43,  39,  40,  41,  51,  52,
	 53,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  87,  88,
	 99,  70, 119, 110, 102, 104, 111, 107, 109, 106, 105, 108, 103,  69,
	 98,  55,  74,  78,  96,  79,  80,  81,  75,  76,  77,  71,  72,  73,
	 82,  83,  86, 127, 116, 117, 183, 184, 185, 186, 187, 188, 189, 190,
	191, 192, 193, 194, 134, 138, 130, 132, 128, 129, 131, 137, 133, 135,
	136, 113, 115, 114,   0,   0,   0, 121,   0,  89,  93, 124,  92,  94,
	 95,   0,   0,   0, 122, 123,  90,  91,  85,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	 29,  42,  56, 125,  97,  54, 100, 126, 164, 166, 165, 163, 161, 115,
	114, 113, 150, 158, 159, 128, 136, 177, 178, 176, 142, 152, 173, 140
};

static unsigned char hidp_mkeyspat[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

class HIDP_CORE {
private:
	void hidp_copy_session(struct hidp_session *session, struct hidp_conninfo *ci);

	/* assemble skb, queue message on @transmit and wake up the session thread */
	int hidp_send_message(struct hidp_session *session, struct socket *sock,
					 struct sk_buff_head *transmit, unsigned char hdr,
					 const unsigned char *data, int size);

	int hidp_send_ctrl_message(struct hidp_session *session,
					  unsigned char hdr, const unsigned char *data,
					  int size);

	int hidp_send_intr_message(struct hidp_session *session,
					  unsigned char hdr, const unsigned char *data,
					  int size);

//	int hidp_input_event_cls(struct input_dev *dev, unsigned int type,
//					unsigned int code, int value);

	void hidp_input_report(struct hidp_session *session, struct sk_buff *skb);
	int hidp_send_report(struct hidp_session *session, struct hid_report *report);
//	int hidp_get_raw_report(struct hid_device *hid,
//			unsigned char report_number,
//			unsigned char *data, size_t count,
//			unsigned char report_type);
//	int hidp_output_raw_report(struct hid_device *hid, unsigned char *data, size_t count,
//			unsigned char report_type);
//	void hidp_idle_timeout(unsigned long arg);
	void hidp_set_timer(struct hidp_session *session);
	void hidp_del_timer(struct hidp_session *session);
	void hidp_process_handshake(struct hidp_session *session, unsigned char param);
	void hidp_process_hid_control(struct hidp_session *session, unsigned char param);

	/* Returns true if the passed-in skb should be freed by the caller. */
	int hidp_process_data(struct hidp_session *session, struct sk_buff *skb, unsigned char param);
	void hidp_recv_ctrl_frame(struct hidp_session *session, struct sk_buff *skb);
	void hidp_recv_intr_frame(struct hidp_session *session, struct sk_buff *skb);
	int hidp_send_frame(struct socket *sock, unsigned char *data, int len);

	/* dequeue message from @transmit and send via @sock */
	void hidp_process_transmit(struct hidp_session *session,
					  struct sk_buff_head *transmit,
					  struct socket *sock);
	int hidp_setup_input(struct hidp_session *session, struct hidp_connadd_req *req);

	/* This function sets up the hid device. It does not add it
	   to the HID system. That is done in hidp_add_connection(). */
	int hidp_setup_hid(struct hidp_session *session, struct hidp_connadd_req *req);
	/* initialize session devices */
	int hidp_session_dev_init(struct hidp_session *session, struct hidp_connadd_req *req);
	/* destroy session devices */
	void hidp_session_dev_destroy(struct hidp_session *session);

	/* add HID/input devices to their underlying bus systems */
	int hidp_session_dev_add(struct hidp_session *session);
	/* remove HID/input devices from their bus systems */
	void hidp_session_dev_del(struct hidp_session *session);
	/* Create new session object */
	int hidp_session_new(struct hidp_session **out, const bdaddr_t *bdaddr,
					struct socket *ctrl_sock,
					struct socket *intr_sock,
					struct hidp_connadd_req *req,
					struct l2cap_conn *conn);
					
	/* increase ref-count of the given session by one */
	void hidp_session_get(struct hidp_session *session);
//	/* release callback */
//	void session_free(struct kref *ref);
	/* decrease ref-count of the given session by one */
	void hidp_session_put(struct hidp_session *session);
	
	/* Search the list of active sessions for a session with target address \bdaddr. */

	struct hidp_session *__hidp_session_find(const bdaddr_t *bdaddr);
	/* Same as __hidp_session_find() but no locks must be held. */
	struct hidp_session *hidp_session_find(const bdaddr_t *bdaddr);
	/* Start session synchronously */
	int hidp_session_start_sync(struct hidp_session *session);
	/* Terminate session thread */
	void hidp_session_terminate(struct hidp_session *session);
	/* Probe HIDP session */
//	int hidp_session_probe(struct l2cap_conn *conn, struct l2cap_user *user);
	/* Remove HIDP session */
//	void hidp_session_remove(struct l2cap_conn *conn, struct l2cap_user *user);
	/* Session Worker */
	void hidp_session_run(struct hidp_session *session);
//	/* HIDP session thread */
//	int hidp_session_thread(void *arg);
	int hidp_verify_sockets(struct socket *ctrl_sock, struct socket *intr_sock);

public:
	int hidp_connection_add(struct hidp_connadd_req *req,
				struct socket *ctrl_sock,
				struct socket *intr_sock);

	int hidp_connection_del(struct hidp_conndel_req *req);
	int hidp_get_connlist(struct hidp_connlist_req *req);
	int hidp_get_conninfo(struct hidp_conninfo *ci);
	
	
	static int hidp_open(struct hid_device *hid);
	static void hidp_close(struct hid_device *hid);
	static int hidp_parse(struct hid_device *hid);
	static int hidp_start(struct hid_device *hid);
	static void hidp_stop(struct hid_device *hid);

        int hidp_input_event_cls(struct input_dev *dev, unsigned int type,
                                        unsigned int code, int value);

        /* HIDP session thread */
        int hidp_session_thread_cls(void *arg);
	     /* release callback */
        void session_free_cls(struct kref *ref);

	void hidp_idle_timeout_cls(unsigned long arg);
        /* Probe HIDP session */
        int hidp_session_probe_cls(struct l2cap_conn *conn, struct l2cap_user *user);
        /* Remove HIDP session */
        void hidp_session_remove_cls(struct l2cap_conn *conn, struct l2cap_user *user);
        int hidp_get_raw_report_cls(struct hid_device *hid,
                        unsigned char report_number,
                        unsigned char *data, size_t count,
                        unsigned char report_type);
        int hidp_output_raw_report_cls(struct hid_device *hid, unsigned char *data, size_t count,
                        unsigned char report_type);

} hidp_core;

int hidp_connection_add(struct hidp_connadd_req *req,
				struct socket *ctrl_sock,
				struct socket *intr_sock);

int hidp_connection_del(struct hidp_conndel_req *req);
int hidp_get_connlist(struct hidp_connlist_req *req);
int hidp_get_conninfo(struct hidp_conninfo *ci);

static int hidp_open(struct hid_device *hid);
static void hidp_close(struct hid_device *hid);
static int hidp_parse(struct hid_device *hid);
static int hidp_start(struct hid_device *hid);
static void hidp_stop(struct hid_device *hid);

/***** HELPER PUBLIC METHODS */
int hidp_input_event(struct input_dev *dev, unsigned int type,
                             unsigned int code, int value);
/* HIDP session thread */
int hidp_session_thread(void *arg);
/* release callback */
void session_free(struct kref *ref);

void hidp_idle_timeout(unsigned long arg);
        /* Probe HIDP session */
        int hidp_session_probe(struct l2cap_conn *conn, struct l2cap_user *user);
        /* Remove HIDP session */
        void hidp_session_remove(struct l2cap_conn *conn, struct l2cap_user *user);
        int hidp_get_raw_report(struct hid_device *hid,
                        unsigned char report_number,
                        unsigned char *data, size_t count,
                        unsigned char report_type);
int hidp_output_raw_report(struct hid_device *hid, unsigned char *data, size_t count,
                   unsigned char report_type);

static struct hid_ll_driver hidp_hid_driver = {
	.parse = hidp_parse,
	.start = hidp_start,
	.stop = hidp_stop,
	.open  = hidp_open,
	.close = hidp_close,
};


/*static int __init hidp_init(void);
static void __exit hidp_exit(void);
*/
