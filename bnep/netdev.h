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
#include <linux/etherdevice.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/l2cap.h>

#include "bnep.h"
#include <c++/end_include.h>


#define BNEP_TX_QUEUE_LEN 20
#define ETH_P_802_3_MIN 0x0600          /* If the value in the ethernet type is less than this value
                                        * then the frame is Ethernet II. Else it is 802.3 */
#define ETH_P_802_3     0x0001          /* Dummy type for 802.3 frames  */
#define ETH_P_AX25      0x0002          /* Dummy protocol id for AX.25  */
#define ETH_P_ALL       0x0003          /* Every packet (be careful!!!) */
#define ETH_P_802_2     0x0004          /* 802.2 frames                 */
#define ETH_P_SNAP      0x0005          /* Internal only                */
#define ETH_P_DDCMP     0x0006          /* DEC DDCMP: Internal only     */
#define ETH_P_WAN_PPP   0x0007          /* Dummy type for WAN PPP frames*/
#define ETH_P_PPP_MP    0x0008          /* Dummy type for PPP MP frames */
#define ETH_P_LOCALTALK 0x0009          /* Localtalk pseudo type        */
#define ETH_P_CAN       0x000C          /* CAN: Controller Area Network */
#define ETH_P_CANFD     0x000D          /* CANFD: CAN flexible data rate*/
#define ETH_P_PPPTALK   0x0010          /* Dummy type for Atalk over PPP*/
#define ETH_P_TR_802_2  0x0011          /* 802.2 frames                 */
#define ETH_P_MOBITEX   0x0015          /* Mobitex (kaz@cafe.net)       */
#define ETH_P_CONTROL   0x0016          /* Card specific control frames */
#define ETH_P_IRDA      0x0017          /* Linux-IrDA                   */
#define ETH_P_ECONET    0x0018          /* Acorn Econet                 */
#define ETH_P_HDLC      0x0019          /* HDLC frames                  */
#define ETH_P_ARCNET    0x001A          /* 1A for ArcNet :-)            */
#define ETH_P_DSA       0x001B          /* Distributed Switch Arch.     */
#define ETH_P_TRAILER   0x001C          /* Trailer switch tagging       */
#define ETH_P_PHONET    0x00F5          /* Nokia Phonet frames          */
#define ETH_P_IEEE802154 0x00F6         /* IEEE802.15.4 frame           */
#define ETH_P_CAIF      0x00F7          /* ST-Ericsson CAIF protocol    */

class BNEP_NETDEV {
	private:	
		#ifdef CONFIG_BT_BNEP_MC_FILTER
			static int bnep_net_mc_filter(struct sk_buff *skb, struct bnep_session *s);
		#endif

		#ifdef CONFIG_BT_BNEP_PROTO_FILTER
			/* Determine ether protocol. Based on eth_type_trans. */
			static u16 bnep_net_eth_proto(struct sk_buff *skb);
			static int bnep_net_proto_filter(struct sk_buff *skb, struct bnep_session *s);
		#endif
	
	public:
		int bnep_net_open(struct net_device *dev);
		int bnep_net_close(struct net_device *dev);
		void bnep_net_set_mc_list(struct net_device *dev);
		int bnep_net_set_mac_addr(struct net_device *dev, void *arg);
		void bnep_net_timeout(struct net_device *dev);
		netdev_tx_t bnep_net_xmit(struct sk_buff *skb, struct net_device *dev);
		void bnep_net_setup(struct net_device *dev);
}bnep_netdev;

void bnep_net_setup(struct net_device *dev);
int bnep_net_open(struct net_device *dev);
static int bnep_net_close(struct net_device *dev);
static netdev_tx_t bnep_net_xmit(struct sk_buff *skb, struct net_device *dev);
static void bnep_net_set_mc_list(struct net_device *dev);
static int bnep_net_set_mac_addr(struct net_device *dev, void *arg);	
static void bnep_net_timeout(struct net_device *dev);
	
static const struct net_device_ops bnep_netdev_ops = {
		.ndo_open            = bnep_net_open,
		.ndo_stop            = bnep_net_close,
		.ndo_start_xmit	     = bnep_net_xmit,
		.ndo_validate_addr   = eth_validate_addr,
		.ndo_set_rx_mode     = bnep_net_set_mc_list,
		.ndo_set_mac_address = bnep_net_set_mac_addr,
		.ndo_tx_timeout      = bnep_net_timeout,
		.ndo_change_mtu	     = eth_change_mtu,

};
