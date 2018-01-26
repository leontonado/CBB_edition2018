#ifndef KNI_STRUCT_H
#define KNI_STRUCT_H
#include <rte_kni.h>
#include <rte_ether.h>
#define KNI_MAX_KTHREAD 32
struct kni_port_params {
	uint8_t port_id;/* Port ID */
	unsigned lcore_rx; /* lcore ID for RX */
	unsigned lcore_tx; /* lcore ID for TX */
	uint32_t nb_lcore_k; /* Number of lcores for KNI multi kernel threads */
	uint32_t nb_kni; /* Number of KNI devices to be created */
	unsigned lcore_k[KNI_MAX_KTHREAD]; /* lcore ID list for kthreads */
	struct rte_kni *kni[KNI_MAX_KTHREAD]; /* KNI context pointers */
} __rte_cache_aligned;
//extern struct rte_mempool * pktmbuf_pool ;
extern struct kni_port_params *kni_port_params_array[RTE_MAX_ETHPORTS];
/* Structure type for recording kni interface specific stats */
struct kni_interface_stats {
	/* number of pkts received from NIC, and sent to KNI */
	uint64_t rx_packets;

	/* number of pkts received from NIC, but failed to send to KNI */
	uint64_t rx_dropped;

	/* number of pkts received from KNI, and sent to NIC */
	uint64_t tx_packets;

	/* number of pkts received from KNI, but failed to send to NIC */
	uint64_t tx_dropped;
};
/* kni device statistics array */
extern struct kni_interface_stats kni_stats[RTE_MAX_ETHPORTS];
extern uint32_t ports_mask;
#endif