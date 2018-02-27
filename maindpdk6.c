#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_udp.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_mbuf.h>
#include <rte_lpm.h>
#include <rte_lpm6.h>
#include <rte_ip.h>
#include <rte_string_fns.h>
#include <rte_ip_frag.h>

#include <unistd.h>
#include <termios.h>
#include <rte_ring.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <rte_errno.h>

#include <time.h> 

#include "allHeaders.h"
//This version runs in the format of frame,which 8 pthread run on 8 individual cores 

//#define RUNMAINDPDK
#ifdef RUNMAINDPDK
/*
 * ******************************************* ip_fragmentation l3fwd begin
 */
static volatile bool force_quit;	

/* MAC updating enabled by default */
// static int mac_updating = 1;

// #define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1 //the  old version
#define RTE_LOGTYPE_IP_FRAG RTE_LOGTYPE_USER1

/* allow max jumbo frame 9.5 KB */
#define JUMBO_FRAME_MAX_SIZE	0x2600

// #define	ROUNDUP_DIV(a, b)	(((a) + (b) - 1) / (b))

/*
 * Default byte size for the IPv6 Maximum Transfer Unit (MTU).
 * This value includes the size of IPv6 header.
 */
#define	IPV4_MTU_DEFAULT	ETHER_MTU
#define	IPV6_MTU_DEFAULT	ETHER_MTU

/*
 * Default payload in bytes for the IPv6 packet.
 */
// #define	IPV4_DEFAULT_PAYLOAD	(IPV4_MTU_DEFAULT - sizeof(struct ipv4_hdr))
// #define	IPV6_DEFAULT_PAYLOAD	(IPV6_MTU_DEFAULT - sizeof(struct ipv6_hdr))

/*
 * Max number of fragments per packet expected - defined by config file.
 */
#define	MAX_PACKET_FRAG RTE_LIBRTE_IP_FRAG_MAX_FRAG

#define NB_MBUF   8192

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/* Configure how many packets ahead to prefetch, when reading packets */
#define PREFETCH_OFFSET	3

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct ether_addr ports_eth_addr[RTE_MAX_ETHPORTS];

#ifndef IPv4_BYTES
#define IPv4_BYTES_FMT "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8
#define IPv4_BYTES(addr) \
		(uint8_t) (((addr) >> 24) & 0xFF),\
		(uint8_t) (((addr) >> 16) & 0xFF),\
		(uint8_t) (((addr) >> 8) & 0xFF),\
		(uint8_t) ((addr) & 0xFF)
#endif

#ifndef IPv6_BYTES
#define IPv6_BYTES_FMT "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"\
                       "%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define IPv6_BYTES(addr) \
	addr[0],  addr[1], addr[2],  addr[3], \
	addr[4],  addr[5], addr[6],  addr[7], \
	addr[8],  addr[9], addr[10], addr[11],\
	addr[12], addr[13],addr[14], addr[15]
#endif

#define IPV6_ADDR_LEN 16

/* mask of enabled ports */
static int enabled_port_mask = 0;

static int rx_queue_per_lcore = 1;

#define MBUF_TABLE_SIZE  (2 * MAX(MAX_PKT_BURST, MAX_PACKET_FRAG))

struct mbuf_table {
	uint16_t len;
	struct rte_mbuf *m_table[MBUF_TABLE_SIZE];
};

struct rx_queue {
	struct rte_mempool *direct_pool;
	struct rte_mempool *indirect_pool;
	struct rte_lpm *lpm;
	struct rte_lpm6 *lpm6;
	uint8_t portid;
};

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
struct lcore_queue_conf {
	uint16_t n_rx_queue;
	uint16_t tx_queue_id[RTE_MAX_ETHPORTS];
	struct rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
	struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.max_rx_pkt_len = JUMBO_FRAME_MAX_SIZE,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 1, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 1, /**< Jumbo Frame Support enabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};


/*
 * IPv4 forwarding table
 */
// struct l3fwd_ipv4_route {
// 	uint32_t ip;
// 	uint8_t  depth;
// 	uint8_t  if_out;
// };

// struct l3fwd_ipv4_route l3fwd_ipv4_route_array[] = {
// 		{IPv4(100,10,0,0), 16, 0},
// 		{IPv4(100,20,0,0), 16, 1},
// 		{IPv4(100,30,0,0), 16, 2},
// 		{IPv4(100,40,0,0), 16, 3},
// 		{IPv4(100,50,0,0), 16, 4},
// 		{IPv4(100,60,0,0), 16, 5},
// 		{IPv4(100,70,0,0), 16, 6},
// 		{IPv4(100,80,0,0), 16, 7},
// };

/*
 * IPv6 forwarding table
 */

// struct l3fwd_ipv6_route {
// 	uint8_t ip[IPV6_ADDR_LEN];
// 	uint8_t depth;
// 	uint8_t if_out;
// };

// static struct l3fwd_ipv6_route l3fwd_ipv6_route_array[] = {
// 	{{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 0},
// 	{{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 1},
// 	{{3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 2},
// 	{{4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 3},
// 	{{5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 4},
// 	{{6,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 5},
// 	{{7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 6},
// 	{{8,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 48, 7},
// };

#define LPM_MAX_RULES         1024
#define LPM6_MAX_RULES         1024
#define LPM6_NUMBER_TBL8S (1 << 16)

struct rte_lpm6_config lpm6_config = {
		.max_rules = LPM6_MAX_RULES,
		.number_tbl8s = LPM6_NUMBER_TBL8S,
		.flags = 0
};

static struct rte_mempool *socket_direct_pool[RTE_MAX_NUMA_NODES];
static struct rte_mempool *socket_indirect_pool[RTE_MAX_NUMA_NODES];
static struct rte_lpm *socket_lpm[RTE_MAX_NUMA_NODES];
static struct rte_lpm6 *socket_lpm6[RTE_MAX_NUMA_NODES];

/*
 *  *******************************************l2fwd end
 */
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 4095
#define NUM_MBUFS2 4095
#define interval 3
static const char *MBUF_POOL = "MBUF_POOL";
// static const char *Mbuf_for_precoding_pool = "Mbuf_for_precoding_pool";

static const char *GenerateData1 = "GenerateData1";
static const char *GenerateData2 = "GenerateData2";
static const char *GenerateData3 = "GenerateData3";
static const char *GenerateData4 = "GenerateData4";
static const char *GenerateData5 = "GenerateData5";
static const char *GenerateData6 = "GenerateData6";
static const char *GenerateData7 = "GenerateData7";
static const char *GenerateData8 = "GenerateData8";

static const char *RetriveData1 = "RetriveData1";
static const char *RetriveData2 = "RetriveData2";
static const char *RetriveData3 = "RetriveData3";
static const char *RetriveData4 = "RetriveData4";
static const char *RetriveData5 = "RetriveData5";
static const char *RetriveData6 = "RetriveData6";
static const char *RetriveData7 = "RetriveData7";
static const char *RetriveData8 = "RetriveData8";

static const char *DatatoRRU = "DatatoRRU";
const unsigned APEP_LEN_DPDK = 512;

// static int i=0; 
struct rte_ring *Ring_GenerateData1;
struct rte_ring *Ring_GenerateData2;
struct rte_ring *Ring_GenerateData3;
struct rte_ring *Ring_GenerateData4;
struct rte_ring *Ring_GenerateData5;
struct rte_ring *Ring_GenerateData6;
struct rte_ring *Ring_GenerateData7;
struct rte_ring *Ring_GenerateData8;

struct rte_ring *Ring_RetriveData1;
struct rte_ring *Ring_RetriveData2;
struct rte_ring *Ring_RetriveData3;
struct rte_ring *Ring_RetriveData4;
struct rte_ring *Ring_RetriveData5;
struct rte_ring *Ring_RetriveData6;
struct rte_ring *Ring_RetriveData7;
struct rte_ring *Ring_RetriveData8;

// struct rte_ring *Ring_DatatoRRU;
struct rte_mempool *mbuf_pool;
// struct rte_mempool *mbuf_precode_pool;
	
volatile int quit = 0;

long int Retrive_DPDK_count = 0;
long int GenerateData_Loop1_count = 0;
long int GenerateData_Loop2_count = 0;
long int GenerateData_Loop3_count = 0;
long int GenerateData_Loop4_count = 0;
long int GenerateData_Loop5_count = 0;
long int GenerateData_Loop6_count = 0;
long int GenerateData_Loop7_count = 0;
long int GenerateData_Loop8_count = 0;
// long int Data_Retrive_Loop_count = 0;
long int Ring_full_count = 0;
long int mbuf_full_count = 0;
long int DatatoTTU_Loopcount = 0;

// complex32 precode_data[4][256*2*16];


int N_CBPS, N_SYM, ScrLength, valid_bits;

struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);
//static int IFFTAndaddWindow_dpdk(__attribute__((unused)) struct rte_mbuf *Data_In);


static int ReadData_Loop(__attribute__((unused)) void *dummy);
static int GenerateData_Loop1(__attribute__((unused)) void *dummy);
static int GenerateData_Loop2(__attribute__((unused)) void *dummy);
static int GenerateData_Loop3(__attribute__((unused)) void *dummy);
static int GenerateData_Loop4(__attribute__((unused)) void *dummy);
static int GenerateData_Loop5(__attribute__((unused)) void *dummy);
static int GenerateData_Loop6(__attribute__((unused)) void *dummy);
static int GenerateData_Loop7(__attribute__((unused)) void *dummy);
static int GenerateData_Loop8(__attribute__((unused)) void *dummy);
//static int Data_Retrive_Loop(__attribute__((unused)) void *dummy);
static int Data_sendto_RRU_loop(__attribute__((unused)) void *dummy);

/*
 * *********************************************** ip fragmentation function begin
 */


/* Send burst of packets on an output interface */
static inline int
send_burst(struct lcore_queue_conf *qconf, uint16_t n, uint8_t port)
{
	struct rte_mbuf **m_table;
	int ret;
	uint16_t queueid;

	queueid = qconf->tx_queue_id[port];
	m_table = (struct rte_mbuf **)qconf->tx_mbufs[port].m_table;

	ret = rte_eth_tx_burst(port, queueid, m_table, n);
	if (unlikely(ret < n)) {
		do {
			rte_pktmbuf_free(m_table[ret]);
		} while (++ret < n);
	}

	return 0;
}

unsigned short checksum(unsigned short *buf,int nword)
{
    unsigned long sum;
    
    for(sum=0;nword>0;nword--)
        sum += *buf++;
    sum = (sum>>16) + (sum&0xffff);
    sum += (sum>>16);
    
    return ~sum;
}

static inline void
l3fwd_simple_forward(struct rte_mbuf *m, struct lcore_queue_conf *qconf,
		uint8_t queueid, uint8_t port_in)
{
	struct rx_queue *rxq;
	uint32_t i, len, next_hop_ipv4;
	//uint8_t next_hop_ipv6, port_out, ipv6;
	uint8_t port_out;
	int32_t len2;

	//ipv6 = 0;
	rxq = &qconf->rx_queue_list[queueid];

	/* by default, send everything back to the source port */
	port_out = port_in;

	/* Remove the Ethernet header and trailer from the input packet */
	//rte_pktmbuf_adj(m, (uint16_t)sizeof(struct ether_hdr));

	//printf("(uint16_t)sizeof(struct ether_hdr) = %d\n",(uint16_t)sizeof(struct ether_hdr));

	/* Build transmission burst */
	len = qconf->tx_mbufs[port_out].len;

	/* if this is an IPv4 packet */
	if (RTE_ETH_IS_IPV4_HDR(m->packet_type)) {
		struct udp_hdr *u_hdr; 
		struct ipv4_hdr *ip_hdr;
		uint32_t ip_dst;
		struct ether_hdr *eth_hdr;
		void * d_addr_bytes;
		/* Read the lookup key (i.e. ip_dst) from the input packet */
		//ip_hdr = rte_pktmbuf_mtod(m, struct ipv4_hdr *);
		//pay attention!!! the mbuf in the beginning don't have ip_hdr, so add a new one befor the data_off
		//m->pkt_len = 22;
		//m->data_len = 22;
		// add udp header
		u_hdr = (struct udp_hdr *)
			rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct udp_hdr));
		if(u_hdr == NULL){
			rte_panic("No headroom in mbuf for upd_hdr.\n");
		}
		m->l4_len = sizeof(struct udp_hdr);
		u_hdr->src_port = htons(0x5561);
		u_hdr->dst_port = htons(0x5562);
		u_hdr->dgram_len = htons(m->pkt_len) ;
		u_hdr->dgram_cksum = 0;

		//add ip header
		ip_hdr = (struct ipv4_hdr *)
			rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct ipv4_hdr));
		if (ip_hdr == NULL) {
			rte_panic("No headroom in mbuf for ip_hdr.\n");
		}
		m->l3_len = sizeof(struct ipv4_hdr);
		ip_hdr->version_ihl = 0x45;
		ip_hdr->type_of_service = 0x90;
		ip_hdr->total_length = htons(m->pkt_len);
		ip_hdr->packet_id = 0;
		ip_hdr->fragment_offset = htons(0x0000); //control the ip fragmentation.
		ip_hdr->time_to_live = 0x21;
		ip_hdr->next_proto_id = 0x11;
		ip_hdr->hdr_checksum = 0;
		ip_hdr->src_addr = htonl(IPv4(196,254,138,109));
		ip_hdr->dst_addr = htonl(IPv4(192,168,1,1));
		ip_dst = rte_be_to_cpu_32(ip_hdr->dst_addr);
		ip_hdr->hdr_checksum = rte_raw_cksum((void *)ip_hdr, 10);
//printf("m->pkt_len = %d\n",m->pkt_len);
		//add eth header
		//eth_hdr = (struct ether_hdr *)
		//	rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct ether_hdr));
		//if (eth_hdr == NULL) {
		//	rte_panic("No headroom in mbuf for eth_hdr.\n");
		//}
		//m->l2_len = sizeof(struct ether_hdr);
		//d_addr_bytes = &eth_hdr->d_addr.addr_bytes[0];
		//*((uint64_t *)d_addr_bytes) = 0xC7AFC96ACC4C;
		//ether_addr_copy(&ports_eth_addr[port_out], &eth_hdr->s_addr);
		//eth_hdr->ether_type = ETHER_TYPE_IPv4;	/**< IPv4 Protocol. */

//unsigned char* frameout = rte_pktmbuf_mtod_offset(m, unsigned char *, 0);
//FILE *c = fopen("frameout.txt","wt");
//int i;
//for(i=0;i<78;i++){
//	//fprintf(c,"%d\t",frameout[i]);
//	fprintf(c,"%c",frameout[i]);
//}
//fclose(c);


		/* Find destination port */
		if (rte_lpm_lookup(rxq->lpm, ip_dst, &next_hop_ipv4) == 0 &&
				(enabled_port_mask & 1 << next_hop_ipv4) != 0) {
			port_out = next_hop_ipv4;

			/* Build transmission burst for new port */
			len = qconf->tx_mbufs[port_out].len;
		}

		/* if we don't need to do any fragmentation */
		if (likely (IPV4_MTU_DEFAULT >= m->pkt_len)) {
			qconf->tx_mbufs[port_out].m_table[len] = m;
			len2 = 1;
		} else {
			len2 = rte_ipv4_fragment_packet(m,
				&qconf->tx_mbufs[port_out].m_table[len],
				(uint16_t)(MBUF_TABLE_SIZE - len),
				IPV4_MTU_DEFAULT,
				rxq->direct_pool, rxq->indirect_pool);

			/* Free input packet */
			rte_pktmbuf_free(m);

			/* If we fail to fragment the packet */
			if (unlikely (len2 < 0))
				return;
		}
	}
	/* else, just forward the packet */
	else {
		qconf->tx_mbufs[port_out].m_table[len] = m;
		len2 = 1;
	}

	for (i = len; i < len + len2; i ++) {
		void *d_addr_bytes;

		m = qconf->tx_mbufs[port_out].m_table[i];
		struct ether_hdr *eth_hdr = (struct ether_hdr *)
			rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct ether_hdr));
		if (eth_hdr == NULL) {
			rte_panic("No headroom in mbuf.\n");
		}

		m->l2_len = sizeof(struct ether_hdr);

		/* 02:00:00:00:00:xx */ // l2fwd mac change function
		d_addr_bytes = &eth_hdr->d_addr.addr_bytes[0];
		//*((uint64_t *)d_addr_bytes) = 0x000000000002 + ((uint64_t)port_out << 40);
		*((uint64_t *)d_addr_bytes) = 0xC7AFC96ACC4C;
		/* src addr */
		ether_addr_copy(&ports_eth_addr[port_out], &eth_hdr->s_addr);
		//if (ipv6)
		//	eth_hdr->ether_type = rte_be_to_cpu_16(ETHER_TYPE_IPv6);
		//else
			eth_hdr->ether_type = rte_be_to_cpu_16(ETHER_TYPE_IPv4);
	}

	len += len2;

	if (likely(len < MAX_PKT_BURST)) {
		qconf->tx_mbufs[port_out].len = (uint16_t)len;
		return;
	}

	/* Transmit packets */
	send_burst(qconf, (uint16_t)len, port_out);
	qconf->tx_mbufs[port_out].len = 0;
}

/* main processing loop */
static int
ip_frag_main_loop(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	int i, j, nb_rx;
	uint8_t portid;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;

	prev_tsc = 0;
	timer_tsc = 0;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_queue == 0) {
		RTE_LOG(INFO, IP_FRAG, "lcore %u has nothing to do\n", lcore_id);
		return -1;
	}

	RTE_LOG(INFO, IP_FRAG, "entering main loop on lcore %u\n", lcore_id);
	printf("lcore_id %u \n", lcore_id);
	/*unsigned master_lcore_id;
	master_lcore_id = rte_get_master_lcore();
	printf("master_lcore_id %u \n", master_lcore_id);*/

	for (i = 0; i < qconf->n_rx_queue; i++) {

		portid = qconf->rx_queue_list[i].portid;
		RTE_LOG(INFO, IP_FRAG, " -- lcoreid=%u portid=%d\n", lcore_id,
				(int) portid);
	}

	while (!force_quit) {
		
		cur_tsc = rte_rdtsc();
		//printf("\ncur_tsc: %20"PRIu64,
		//	   cur_tsc);
		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		//printf("\ndiff_tsc: %20"PRIu64,
		//	   diff_tsc);

		if (unlikely(diff_tsc > drain_tsc)) {

			/*
			 * This could be optimized (use queueid instead of
			 * portid), but it is not called so often
			 */

			//for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
			//	if (qconf->tx_mbufs[portid].len == 0)
			//		continue;
			//	send_burst(&lcore_queue_conf[lcore_id],
			//		   qconf->tx_mbufs[portid].len,
			//		   portid);
			//	qconf->tx_mbufs[portid].len = 0;
			//}

			prev_tsc = cur_tsc;
		}

		/*
		 * Read packet from RX queues
		 */
		//printf("qconf->n_rx_queue = %d\n",qconf->n_rx_queue);
		/*for (i = 0; i < qconf->n_rx_queue; i++) {

			portid = qconf->rx_queue_list[i].portid;
			nb_rx = rte_eth_rx_burst(portid, 0, pkts_burst,
						 MAX_PKT_BURST);
			//printf("nb_rx = %d\n",nb_rx);
			// Prefetch first packets 
			for (j = 0; j < PREFETCH_OFFSET && j < nb_rx; j++) {
				rte_prefetch0(rte_pktmbuf_mtod(
						pkts_burst[j], void *));
				printf("-- lcoreid=%u nb_rx = %d\n", lcore_id, nb_rx);
			}

			// Prefetch and forward already prefetched packets 
			for (j = 0; j < (nb_rx - PREFETCH_OFFSET); j++) {
				rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[
						j + PREFETCH_OFFSET], void *));
				l3fwd_simple_forward(pkts_burst[j], qconf, i, portid);
			}

			// Forward remaining prefetched packets 
			for (; j < nb_rx; j++) {
				l3fwd_simple_forward(pkts_burst[j], qconf, i, portid);
			}
		}*/
	}
}

/* display usage */
static void
print_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n",
	       prgname);
}

static int
parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

static int
parse_nqueue(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n == 0)
		return -1;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return -1;

	return n;
}


/* Parse the argument given in the command line of the application */
static int
parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		//{ "mac-updating", no_argument, &mac_updating, 1},
		//{ "no-mac-updating", no_argument, &mac_updating, 0},
		{NULL, 0, 0, 0}
	};

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, "p:q:",
				  lgopts, &option_index)) != EOF) {

		switch (opt) {
		/* portmask */
		case 'p':
			enabled_port_mask = parse_portmask(optarg);
			if (enabled_port_mask < 0) {
				printf("invalid portmask\n");
				print_usage(prgname);
				return -1;
			}
			break;

		/* nqueue */
		case 'q':
			rx_queue_per_lcore = parse_nqueue(optarg);
			if (rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				print_usage(prgname);
				return -1;
			}
			break;

		/* long options */
		case 0:
			print_usage(prgname);
			return -1;

		default:
			print_usage(prgname);
			return -1;
		}
	}

	if (enabled_port_mask == 0) {
		printf("portmask not specified\n");
		print_usage(prgname);
		return -1;
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 0; /* reset getopt lib */
	return ret;
}

static void
print_ethaddr(const char *name, struct ether_addr *eth_addr)
{
	char buf[ETHER_ADDR_FMT_SIZE];
	ether_format_addr(buf, ETHER_ADDR_FMT_SIZE, eth_addr);
	printf("%s%s", name, buf);
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
						(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("\ndone\n");
		}
	}
}

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
		quit = 1;
	}
}

/*
 * *********************************************** ip fragmentation function end
 */

struct timespec diff(struct timespec start, struct timespec end)
{
    struct  timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
         temp.tv_sec = end.tv_sec-start.tv_sec-1;
         temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } 
    else {
         temp.tv_sec = end.tv_sec-start.tv_sec;
         temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
     return temp;
}


static int InitData(unsigned char** p_databits)
{
	FILE *fp=fopen("send_din_dec.txt","rt");
	unsigned char *databits=(unsigned char*)malloc(APEP_LEN_DPDK*sizeof(unsigned char));
	*p_databits = databits;
	if(databits == NULL){
		printf("error");
		return 0;
	}
	unsigned int datatmp=0;
	int i;
	for(i=0;i<APEP_LEN_DPDK;i++){
	    fscanf(fp,"%ud",&datatmp);
	    databits[i]=datatmp&0x000000FF;
	}
	fclose(fp);
	return 0;
}


static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in) 
{
	if(time_test_flag==0){
		time_test_flag = 1;
		clock_gettime(CLOCK_REALTIME, &time1); //CLOCK_REALTIME:系统实时时间   
		//CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
		//CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
		//CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	}
	rte_memcpy(rte_pktmbuf_mtod(Data_out,unsigned char *), Data_in, APEP_LEN_DPDK);
	return 0;
}


static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In) 
{	
	unsigned char *databits = rte_pktmbuf_mtod(Data_In, unsigned char *);
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	GenDataAndScramble(data_scramble, ScrLength, databits, valid_bits);	
	return 0;
}


static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	int CodeLength = N_SYM*N_CBPS/N_STS;
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	BCC_encoder_OPT(data_scramble, ScrLength, N_SYM, &BCCencodeout, CodeLength);
	return 0;
}


static int modulate_DPDK(__attribute__((unused)) struct rte_mbuf *Data_In)
{
	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	modulate_mapping(BCCencodeout,&subcar_map_data);
	FILE *k=fopen("modulate_data.txt","w");
	printStreamToFile_float(subcar_map_data,2048,k);
	fclose(k);
	return 0;
}	


static int CSD_encode_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	int i;
	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *,0 );
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	for(i=0;i<N_STS;i++){
		__Data_CSD_aux(&subcar_map_data, N_SYM, &csd_data,i);
	}
	return 0;
}

/*
static int IFFTAndaddWindow_dpdk(__attribute__((unused)) struct rte_mbuf *Data_In)
{
	int i;
	//IFFTAndaddWindow_dpdk_count++;
	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *,0 );
	complex32 *IFFT_data = rte_pktmbuf_mtod_offset(Data_In,complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15);
	csd_data_IDFT(csd_data,IFFT_data,N_SYM);
	/*FILE *k=fopen("IFFT_data.txt","w");
	printStreamToFile_float(IFFT_data,5120,k);
	fclose(k);
	return 0;
}
*/

static int Data_sendto_RRU_loop(__attribute__((unused)) void *dummy)
{
	void *Data_In_GenerateData=NULL;
	unsigned portid = 1;
	struct lcore_queue_conf *qconf;
	struct rte_mbuf * m ;
	qconf = &lcore_queue_conf[0];

	while (!quit)
	{
		if (rte_ring_dequeue(Ring_RetriveData1, &Data_In_GenerateData) >= 0 || 
			rte_ring_dequeue(Ring_RetriveData2, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData3, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData4, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData5, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData6, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData7, &Data_In_GenerateData) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData8, &Data_In_GenerateData) >= 0 ) {
			//Data_sendto_RRU();
			DatatoTTU_Loopcount++;
			// prefetch0 improve cache hit rate
			m = (struct rte_mbuf *)Data_In_GenerateData;
			m->packet_type = (uint32_t)RTE_PTYPE_L3_IPV4; 	//this is an IPv4 packet 
			m->data_len = sizeof(complex32) * 2048;
			m->pkt_len = sizeof(complex32) * 2048; 
			m->data_off += RTE_MBUF_DEFAULT_BUF_SIZE*15;
			rte_prefetch0(rte_pktmbuf_mtod(m, void *));
			// send
			l3fwd_simple_forward(m, qconf, 0, 0);
			if(DatatoTTU_Loopcount >= 100)	
			{
				quit = 1;
				clock_gettime(CLOCK_REALTIME, &time2);
				time_diff = diff(time1,time2);
				printf("Retrive_DPDK_count = %ld\n", Retrive_DPDK_count);
				printf("DatatoTTU_Loopcount = %ld\n", DatatoTTU_Loopcount);
				printf("Start time # %.24s %ld Nanoseconds \n",ctime(&time1.tv_sec), time1.tv_nsec);
				printf("Stop time # %.24s %ld Nanoseconds \n",ctime(&time2.tv_sec), time2.tv_nsec);
				printf("Running time # %ld.%ld Seconds \n",time_diff.tv_sec, time_diff.tv_nsec);
				printf("GenerateData_Loop1_count = %ld\n", GenerateData_Loop1_count);
				printf("GenerateData_Loop2_count = %ld\n", GenerateData_Loop2_count);
				printf("GenerateData_Loop3_count = %ld\n", GenerateData_Loop3_count);
				printf("GenerateData_Loop4_count = %ld\n", GenerateData_Loop4_count);
				printf("GenerateData_Loop5_count = %ld\n", GenerateData_Loop5_count);
				printf("GenerateData_Loop6_count = %ld\n", GenerateData_Loop6_count);
				printf("GenerateData_Loop7_count = %ld\n", GenerateData_Loop7_count);
				printf("GenerateData_Loop8_count = %ld\n", GenerateData_Loop8_count);	

				//printf("Data_Retrive_Loop_count = %ld\n", Data_Retrive_Loop_count);
				printf("Ring_full_count = %ld\n",Ring_full_count);
				printf("mbuf_full_count = %ld\n",mbuf_full_count);
				//free(dest);
			}
			//the old 
			//rte_prefetch0(rte_pktmbuf_mtod((struct rte_mbuf *)Data_In_GenerateData, void *));
			//l2fwd_simple_forward((struct rte_mbuf *)Data_In_GenerateData, portid); //when send success The mbuf will free automaticly.
			//the old old 
			//rte_mempool_put(((struct rte_mbuf *)Data_In_GenerateData)->pool, Data_In_GenerateData);
		}
		else {	
			usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int check_empty_Ring()
{
	if (rte_ring_empty(Ring_RetriveData1) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData2) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData3) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData4) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData5) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData6) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData7) == 1 ) 
		return 1;
	if (rte_ring_empty(Ring_RetriveData8) == 1 ) 
		return 1;
	 
	return 0 ;			
}

// static int Data_Retrive_Loop(__attribute__((unused)) void *dummy) 
// {
// 	/*__attribute__((unused)) struct rte_mbuf *data1 = NULL;
// 	__attribute__((unused)) struct rte_mbuf *data2 = NULL;	
// 	__attribute__((unused)) struct rte_mbuf *data3 = NULL;
// 	__attribute__((unused)) struct rte_mbuf *data4 = NULL;*/
// 	__attribute__((unused)) struct rte_mbuf *data = NULL;
// 	void *Data_User_1=NULL;
// 	void *Data_User_2=NULL;
// 	void *Data_User_3=NULL;
// 	void *Data_User_4=NULL;
// 	void *Data_User_5=NULL;
// 	void *Data_User_6=NULL;
// 	void *Data_User_7=NULL;
// 	void *Data_User_8=NULL;
// 	complex32 *User_1 = NULL;
// 	complex32 *User_2 = NULL;
// 	complex32 *User_3 = NULL;
// 	complex32 *User_4 = NULL;
// 	complex32 *User_5 = NULL;
// 	complex32 *User_6 = NULL;
// 	complex32 *User_7 = NULL;
// 	complex32 *User_8 = NULL;

// 	int i,j;
// 	int k = 0;
// 	complex32 *dest = NULL;
// 	complex32 X[8] = {{0,0}};
// 	//get the precoding matrix
// 	complex32 h[8][8];
// 	char h_updating;
// 	srand((unsigned)time(NULL));
// 	h_updating = 1;
// 	for(i=0; i<8; i++){
// 		for (j = 0; j < 8; j++){
// 			h[i][j].real = (double)(rand() / (double)RAND_MAX) * (0x1 << 13);
// 			h[i][j].imag = (double)(rand() / (double)RAND_MAX) * (0x1 << 13);
// 		}
// 	}

// 	while (!quit)
// 	{
// 		if (!check_empty_Ring()){
// 			Retrive_DPDK_count++;
// 			while(rte_ring_dequeue(Ring_RetriveData1, &Data_User_1) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData2, &Data_User_2) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData3, &Data_User_3) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData4, &Data_User_4) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData5, &Data_User_5) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData6, &Data_User_6) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData7, &Data_User_7) < 0);
// 			while(rte_ring_dequeue(Ring_RetriveData8, &Data_User_8) < 0);
// 			User_1 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_1, complex32 *,0 );
// 			User_2 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_2, complex32 *,0 );
// 			User_3 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_3, complex32 *,0 );
// 			User_4 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_4, complex32 *,0 );
// 			User_5 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_5, complex32 *,0 );
// 			User_6 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_6, complex32 *,0 );
// 			User_7 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_7, complex32 *,0 );
// 			User_8 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_8, complex32 *,0 );
// 			//Precode_processing
// 			k = 0;	
// 			while(k < 2){
// 				data = rte_pktmbuf_alloc(mbuf_precode_pool);
// 				data->packet_type = (uint32_t)RTE_PTYPE_L3_IPV4; 	//this is an IPv4 packet 
// 				data->data_len = sizeof(complex32) * 8 * 1024;
// 				data->pkt_len = sizeof(complex32) * 8 * 1024;

// 				for(i = subcar*N_SYM*N_STS/2*k;i < subcar*N_SYM*N_STS/2*(k+1);i++){
// 					X[0] = *(User_1 + i);
// 					X[1] = *(User_2 + i);
// 					X[2] = *(User_3 + i);
// 					X[3] = *(User_4 + i);
// 					X[5] = *(User_5 + i);
// 					X[6] = *(User_6 + i);
// 					X[7] = *(User_7 + i);
// 					X[8] = *(User_8 + i);
// 					dest = rte_pktmbuf_mtod_offset(data, complex32 *, 8*(i-subcar*N_SYM*N_STS/8*k));
// 					//Matrix_Mult_AVX2_8(h,X,dest);	

// 					Matrix_Mult_AVX2_8_opt(h,X,dest,h_updating);
// 					h_updating = 0;				
// 				}


// // dest = rte_pktmbuf_mtod_offset(data, complex32 *, 0);
// // if(k==0){
// // 	FILE *ts =fopen("precoding_data_uint16_1.txt","wt");
// // 	//for(i=0;i<8 * 1024;i++)
// // 	printStreamToFile(dest, 8 * 1024, ts);
// // 	fclose(ts);
// // }
// // else{
// // 	FILE *ts =fopen("precoding_data_uint16_2.txt","wt");
// // 	//for(i=0;i<8 * 1024;i++)
// // 	printStreamToFile(dest, 8 * 1024, ts);
// // 	fclose(ts);
// // }

// //for(i=0;i<sizeof(complex32) * 8 * 1024;i++)
// //	fprintf(ts,"%c",testdata[i]);
// 	//printStreamToFile(dest, 8 * 1024, ts);

// 				//memcpy(precode_data[k],rte_pktmbuf_mtod_offset(data, complex32 *,0),subcar*N_SYM*N_STS/4*16);
// 				rte_ring_enqueue(Ring_DatatoRRU, data);
// 				k++;
// 			}
			
// 			/*FILE *ts =fopen("precoding_data.txt","w");
// 				for(i=0;i<4;i++)
// 					printStreamToFile(precode_data[i],subcar*N_SYM*N_STS/4*16,ts);	*/
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_1)->pool, Data_User_1);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_2)->pool, Data_User_2);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_3)->pool, Data_User_3);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_4)->pool, Data_User_4);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_5)->pool, Data_User_5);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_6)->pool, Data_User_6);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_7)->pool, Data_User_7);
// 			rte_mempool_put(((struct rte_mbuf *)Data_User_8)->pool, Data_User_8);
// 			Data_User_1 = NULL;
// 			Data_User_2 = NULL;
// 			Data_User_3 = NULL;
// 			Data_User_4 = NULL;
// 			Data_User_5 = NULL;
// 			Data_User_6 = NULL;
// 			Data_User_7 = NULL;
// 			Data_User_8 = NULL;	
// 			User_1 = NULL;
// 			User_2 = NULL;
// 			User_3 = NULL;
// 			User_4 = NULL;
// 			User_5 = NULL;
// 			User_6 = NULL;
// 			User_7 = NULL;
// 			User_8 = NULL;
// 		}
// 		else {
// 			Data_Retrive_Loop_count++;
// 			usleep(1000);
// 			continue;
// 		}
		

// 	}
// 	return 0;
// }

static int GenerateData_Loop1(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData1, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData1, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop1_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop2(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData2, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData2, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop2_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop3(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData3, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData3, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop3_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop4(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData4, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData4, Data_In_GenerateData);
		}
		else
		{	
			GenerateData_Loop4_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop5(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData5, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData5, Data_In_GenerateData);
		}
		else
		{	
			GenerateData_Loop5_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop6(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData6, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData6, Data_In_GenerateData);
		}
		else
		{	
			GenerateData_Loop6_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop7(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData7, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData7, Data_In_GenerateData);
		}
		else
		{	
			GenerateData_Loop7_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop8(__attribute__((unused)) void *dummy) 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData8, &Data_In_GenerateData) >= 0 )
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData8, Data_In_GenerateData);
		}
		else
		{	
			GenerateData_Loop8_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int FindEmptyRing()
{
	if(rte_ring_full(Ring_GenerateData1)==0)
		return 1;
	if(rte_ring_full(Ring_GenerateData2)==0)
		return 2;
	if(rte_ring_full(Ring_GenerateData3)==0)
		return 3;
	if(rte_ring_full(Ring_GenerateData4)==0)
		return 4;
	if(rte_ring_full(Ring_GenerateData5)==0)
		return 5;
	if(rte_ring_full(Ring_GenerateData6)==0)
		return 6;
	if(rte_ring_full(Ring_GenerateData7)==0)
		return 7;
	if(rte_ring_full(Ring_GenerateData8)==0)
		return 8;

	return 0;
}

static int PutInEmptyRing(struct rte_mbuf *Data,int n)
{
	switch(n){
		case 1:	rte_ring_enqueue(Ring_GenerateData1, Data);
			break;
		case 2:	rte_ring_enqueue(Ring_GenerateData2, Data);
			break;
		case 3:	rte_ring_enqueue(Ring_GenerateData3, Data);
			break;
		case 4:	rte_ring_enqueue(Ring_GenerateData4, Data);
			break;
		case 5:	rte_ring_enqueue(Ring_GenerateData5, Data);
			break;
		case 6:	rte_ring_enqueue(Ring_GenerateData6, Data);
			break;
		case 7:	rte_ring_enqueue(Ring_GenerateData7, Data);
			break;
		case 8:	rte_ring_enqueue(Ring_GenerateData8, Data);
			break;
		//default:printf("Not empty ring\n");
	}

	return 0;
}

static int ReadData_Loop(__attribute__((unused)) void *dummy) 
{
	struct rte_mbuf *Data =NULL;
	unsigned char* Data_in =NULL;
	int dis_count = 0;
	int n;
	InitData(&Data_in);
	while (!quit){
		n=FindEmptyRing();
		if(n!=0){
			Data = rte_pktmbuf_alloc(mbuf_pool);
			if (Data != NULL){
				ReadData(Data, Data_in);
				//rte_ring_enqueue(Ring_Beforescramble, Data);
				dis_count++;
				if(dis_count >= interval*8){
					dis_count = 0;
				}

				if(dis_count >= 0 && dis_count < interval){
				    	if(rte_ring_full(Ring_GenerateData1)==0)
				         		rte_ring_enqueue(Ring_GenerateData1, Data);
				     	else
				     		PutInEmptyRing(Data,n);
				}	
				else if(dis_count >= interval && dis_count < interval*2){
				    	if(rte_ring_full(Ring_GenerateData2)==0)
				       		rte_ring_enqueue(Ring_GenerateData2, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*2 && dis_count < interval*3){
				    	if(rte_ring_full(Ring_GenerateData3)==0)
				       		rte_ring_enqueue(Ring_GenerateData3, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*3 && dis_count < interval*4){
				    	if(rte_ring_full(Ring_GenerateData4)==0)
				       		rte_ring_enqueue(Ring_GenerateData4, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*4 && dis_count < interval*5){
				    	if(rte_ring_full(Ring_GenerateData5)==0)
				       		rte_ring_enqueue(Ring_GenerateData5, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*5 && dis_count < interval*6){
				    	if(rte_ring_full(Ring_GenerateData6)==0)
				       		rte_ring_enqueue(Ring_GenerateData6, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*6 && dis_count < interval*7){
				    	if(rte_ring_full(Ring_GenerateData7)==0)
				       		rte_ring_enqueue(Ring_GenerateData7, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
				else if(dis_count >= interval*7 && dis_count < interval*8){
				    	if(rte_ring_full(Ring_GenerateData8)==0)
				       		rte_ring_enqueue(Ring_GenerateData8, Data);
				    	else
				    		PutInEmptyRing(Data,n);
				}
			}
			else{			 
			    mbuf_full_count++;
			    //usleep(100000);
			    continue;
			}
		}
		else{
			Ring_full_count++;
			continue;
		}
	}   
	return 0;
}

// static int
// init_routing_table(void)
// {
// 	struct rte_lpm *lpm;
// 	struct rte_lpm6 *lpm6;
// 	int socket, ret;
// 	unsigned i;

// 	for (socket = 0; socket < RTE_MAX_NUMA_NODES; socket++) {
// 		if (socket_lpm[socket]) {
// 			lpm = socket_lpm[socket];
// 			/* populate the LPM table */
// 			for (i = 0; i < RTE_DIM(l3fwd_ipv4_route_array); i++) {
// 				ret = rte_lpm_add(lpm,
// 					l3fwd_ipv4_route_array[i].ip,
// 					l3fwd_ipv4_route_array[i].depth,
// 					l3fwd_ipv4_route_array[i].if_out);

// 				if (ret < 0) {
// 					RTE_LOG(ERR, IP_FRAG, "Unable to add entry %i to the l3fwd "
// 						"LPM table\n", i);
// 					return -1;
// 				}

// 				RTE_LOG(INFO, IP_FRAG, "Socket %i: adding route " IPv4_BYTES_FMT
// 						"/%d (port %d)\n",
// 					socket,
// 					IPv4_BYTES(l3fwd_ipv4_route_array[i].ip),
// 					l3fwd_ipv4_route_array[i].depth,
// 					l3fwd_ipv4_route_array[i].if_out);
// 			}
// 		}

// 		if (socket_lpm6[socket]) {
// 			lpm6 = socket_lpm6[socket];
// 			/* populate the LPM6 table */
// 			for (i = 0; i < RTE_DIM(l3fwd_ipv6_route_array); i++) {
// 				ret = rte_lpm6_add(lpm6,
// 					l3fwd_ipv6_route_array[i].ip,
// 					l3fwd_ipv6_route_array[i].depth,
// 					l3fwd_ipv6_route_array[i].if_out);

// 				if (ret < 0) {
// 					RTE_LOG(ERR, IP_FRAG, "Unable to add entry %i to the l3fwd "
// 						"LPM6 table\n", i);
// 					return -1;
// 				}

// 				RTE_LOG(INFO, IP_FRAG, "Socket %i: adding route " IPv6_BYTES_FMT
// 						"/%d (port %d)\n",
// 					socket,
// 					IPv6_BYTES(l3fwd_ipv6_route_array[i].ip),
// 					l3fwd_ipv6_route_array[i].depth,
// 					l3fwd_ipv6_route_array[i].if_out);
// 			}
// 		}
// 	}
// 	return 0;
// }

static int
init_mem(void)
{
	char buf[PATH_MAX];
	struct rte_mempool *mp;
	struct rte_lpm *lpm;
	struct rte_lpm6 *lpm6;
	struct rte_lpm_config lpm_config;
	int socket;
	unsigned lcore_id;

	/* traverse through lcores and initialize structures on each socket */

	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {

		if (rte_lcore_is_enabled(lcore_id) == 0)
			continue;

		socket = rte_lcore_to_socket_id(lcore_id);

		if (socket == SOCKET_ID_ANY)
			socket = 0;

		if (socket_direct_pool[socket] == NULL) {
			RTE_LOG(INFO, IP_FRAG, "Creating direct mempool on socket %i\n",
					socket);
			snprintf(buf, sizeof(buf), "pool_direct_%i", socket);

			mp = rte_pktmbuf_pool_create(buf, NB_MBUF, 32,
				0, RTE_MBUF_DEFAULT_BUF_SIZE, socket);
			if (mp == NULL) {
				RTE_LOG(ERR, IP_FRAG, "Cannot create direct mempool\n");
				return -1;
			}
			socket_direct_pool[socket] = mp;
		}

		if (socket_indirect_pool[socket] == NULL) {
			RTE_LOG(INFO, IP_FRAG, "Creating indirect mempool on socket %i\n",
					socket);
			snprintf(buf, sizeof(buf), "pool_indirect_%i", socket);

			mp = rte_pktmbuf_pool_create(buf, NB_MBUF, 32, 0, 0,
				socket);
			if (mp == NULL) {
				RTE_LOG(ERR, IP_FRAG, "Cannot create indirect mempool\n");
				return -1;
			}
			socket_indirect_pool[socket] = mp;
		}

		if (socket_lpm[socket] == NULL) {
			RTE_LOG(INFO, IP_FRAG, "Creating LPM table on socket %i\n", socket);
			snprintf(buf, sizeof(buf), "IP_FRAG_LPM_%i", socket);

			lpm_config.max_rules = LPM_MAX_RULES;
			lpm_config.number_tbl8s = 256;
			lpm_config.flags = 0;

			lpm = rte_lpm_create(buf, socket, &lpm_config);
			if (lpm == NULL) {
				RTE_LOG(ERR, IP_FRAG, "Cannot create LPM table\n");
				return -1;
			}
			socket_lpm[socket] = lpm;
		}

		if (socket_lpm6[socket] == NULL) {
			RTE_LOG(INFO, IP_FRAG, "Creating LPM6 table on socket %i\n", socket);
			snprintf(buf, sizeof(buf), "IP_FRAG_LPM_%i", socket);

			lpm6 = rte_lpm6_create(buf, socket, &lpm6_config);
			if (lpm6 == NULL) {
				RTE_LOG(ERR, IP_FRAG, "Cannot create LPM table\n");
				return -1;
			}
			socket_lpm6[socket] = lpm6;
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	/* ip fragment parameter */
	struct lcore_queue_conf *qconf;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf *txconf;
	struct rx_queue *rxq;
	int socket, ret;
	uint8_t nb_ports;
	uint16_t queueid = 0;
	unsigned lcore_id = 0, rx_lcore_id = 0;
	uint32_t n_tx_queue, nb_lcores;
	uint8_t portid;

	const unsigned flags = 0;
	const unsigned ring_size = 4096;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
	//int ret;
	// 运行一次得到preamble和HeLTF.
	//generatePreambleAndHeLTF_csd();
	// 运行一次得到比特干扰码表。
	Creatnewchart();
	// 运行一次得到BCC编码表。
	init_BCCencode_table();
	
	// 运行一次得到CSD表。
	//initcsdTableForHeLTF();
	// 初始化函数，计算OFDM符号个数，字节长度
	//int N_CBPS, N_SYM, ScrLength, valid_bits;
   	GenInit(&N_CBPS, &N_SYM, &ScrLength, &valid_bits);

   	// 运行一次得到生成导频的分流交织表
	initial_streamwave_table(N_SYM);
	init_mapping_table();
	///////////////////////////////////////////////////////////////////////////////////
	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid arguments\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No ports found!\n");
	printf("nb_ports = %d\n", nb_ports);

	nb_lcores = rte_lcore_count();

	/* initialize structures (mempools, lpm etc.) */
	if (init_mem() < 0)
		rte_panic("Cannot initialize memory structures!\n");

	/* check if portmask has non-existent ports */
	if (enabled_port_mask & ~(RTE_LEN2MASK(nb_ports, unsigned)))
		rte_exit(EXIT_FAILURE, "Non-existent ports in portmask!\n");

	/* create the mbuf pool */
	/*l2fwd_pktmbuf_pool = rte_socket_direct_nb_portspoolpktmbuf_pool_create("mbuf_pool", NB_MBUF,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");*/
	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create(MBUF_POOL, NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*30, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	// mbuf_precode_pool = rte_pktmbuf_pool_create(Mbuf_for_precoding_pool,NUM_MBUFS2,MBUF_CACHE_SIZE,
	// 	0,RTE_MBUF_DEFAULT_BUF_SIZE*18,rte_socket_id());
	// if (mbuf_precode_pool == NULL)
	// 	rte_exit(EXIT_FAILURE, "Cannot create mbuf pool for precoding:%s\n",rte_strerror(rte_errno));

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	//for (portid = 0; portid < nb_ports; portid++) {
	//	/* skip ports that are not enabled */
	//	if ((enabled_port_mask & (1 << portid)) == 0)
	//		continue;
//
	//	if (nb_ports_in_mask % 2) {
	//		l2fwd_dst_ports[portid] = last_port;
	//		l2fwd_dst_ports[last_port] = portid;
	//	}
	//	else
	//		last_port = portid;
//
	//	nb_ports_in_mask++;
//
	//	rte_eth_dev_info_get(portid, &dev_info);
	//}
	//if (nb_ports_in_mask % 2) {
	//	printf("Notice: odd number of ports in portmask.\n");
	//	l2fwd_dst_ports[last_port] = last_port; 
	//}
//
	//rx_lcore_id = 0;
	//qconf = NULL;
//
	///* Initialize the port/queue configuration of each logical core */
	//for (portid = 0; portid < nb_ports; portid++) {
	//	/* skip ports that are not enabled */
	//	if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
	//		continue;
//
	//	/* get the lcore_id for this port */
	//	while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
	//	       lcore_queue_conf[rx_lcore_id].n_rx_port ==
	//	       l2fwd_rx_queue_per_lcore) {
	//		rx_lcore_id++;
	//		if (rx_lcore_id >= RTE_MAX_LCORE)
	//			rte_exit(EXIT_FAILURE, "Not enough cores\n");
	//	}
//
	//	if (qconf != &lcore_queue_conf[rx_lcore_id])
	//		/* Assigned a new logical core in the loop above. */
	//		qconf = &lcore_queue_conf[rx_lcore_id];
//
	//	qconf->rx_port_list[qconf->n_rx_port] = portid;
	//	qconf->n_rx_port++;
	//	printf("Lcore %u: RX port %u\n", rx_lcore_id, (unsigned) portid);
	//}
//
	//nb_ports_available = nb_ports;

	/* Initialise each port */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %d\n", portid);
			continue;
		}

		qconf = &lcore_queue_conf[rx_lcore_id];

		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       qconf->n_rx_queue == (unsigned)rx_queue_per_lcore) {

			rx_lcore_id ++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");

			qconf = &lcore_queue_conf[rx_lcore_id];
		}

		socket = (int) rte_lcore_to_socket_id(rx_lcore_id);
		if (socket == SOCKET_ID_ANY)
			socket = 0;

		rxq = &qconf->rx_queue_list[qconf->n_rx_queue];
		rxq->portid = portid;
		rxq->direct_pool = socket_direct_pool[socket];
		rxq->indirect_pool = socket_indirect_pool[socket];
		rxq->lpm = socket_lpm[socket];
		rxq->lpm6 = socket_lpm6[socket];
		qconf->n_rx_queue++;

		/* init port */
		printf("Initializing port %d on lcore %u...\n", portid,
		       rx_lcore_id);
		fflush(stdout);

		n_tx_queue = nb_lcores;
		if (n_tx_queue > MAX_TX_QUEUE_PER_PORT)
			n_tx_queue = MAX_TX_QUEUE_PER_PORT;
		ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
		if (ret < 0) {////////////////////////////////////////////////////////////////////////////////bug  bug !!!
			printf("\n");
			printf("bug  bug !!! how could i kill it\n");
			rte_exit(EXIT_FAILURE, "Cannot configure device: "
				"err=%d, port=%d\n",
				ret, portid);

		}

		/* init one RX queue */
		fflush(stdout);
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     socket, NULL,
					     socket_direct_pool[socket]);
		if (ret < 0) {
			printf("\n");
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: "
				"err=%d, port=%d\n",
				ret, portid);
		}

		rte_eth_macaddr_get(portid,&ports_eth_addr[portid]);
		print_ethaddr(" Address:", &ports_eth_addr[portid]);
		printf("\n");

		/* init one TX queue per couple (lcore,port) */
		queueid = 0;
		for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
			if (rte_lcore_is_enabled(lcore_id) == 0)
				continue;

			socket = (int) rte_lcore_to_socket_id(lcore_id);
			printf("txq=%u,%d ", lcore_id, queueid);
			fflush(stdout);

			rte_eth_dev_info_get(portid, &dev_info);
			txconf = &dev_info.default_txconf;
			txconf->txq_flags = 0;
			ret = rte_eth_tx_queue_setup(portid, queueid, nb_txd,
						     socket, txconf);
			if (ret < 0) {
				printf("\n");
				rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: "
					"err=%d, port=%d\n", ret, portid);
			}

			qconf = &lcore_queue_conf[lcore_id];
			qconf->tx_queue_id[portid] = queueid;
			//queueid++;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 		}

		printf("\n");
	}

	printf("\n");

	/* start ports */
	for (portid = 0; portid < nb_ports; portid++) {
		if ((enabled_port_mask & (1 << portid)) == 0) {
			continue;
		}
		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start: err=%d, port=%d\n",
				ret, portid);

		rte_eth_promiscuous_enable(portid);
	}

	// if (init_routing_table() < 0)
	// 	rte_exit(EXIT_FAILURE, "Cannot init routing table\n");

	check_all_ports_link_status((uint8_t)nb_ports, enabled_port_mask);

	Ring_GenerateData1 = rte_ring_create(GenerateData1 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData2 = rte_ring_create(GenerateData2 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData3 = rte_ring_create(GenerateData3 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData4 = rte_ring_create(GenerateData4 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData5 = rte_ring_create(GenerateData5 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData6 = rte_ring_create(GenerateData6 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData7 = rte_ring_create(GenerateData7 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData8 = rte_ring_create(GenerateData8 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);

	Ring_RetriveData1 = rte_ring_create(RetriveData1 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData2 = rte_ring_create(RetriveData2 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData3 = rte_ring_create(RetriveData3 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData4 = rte_ring_create(RetriveData4 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData5 = rte_ring_create(RetriveData5 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData6 = rte_ring_create(RetriveData6 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData7 = rte_ring_create(RetriveData7 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData8 = rte_ring_create(RetriveData8 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);

	// Ring_DatatoRRU = rte_ring_create(DatatoRRU, ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);//ring_size  over is 256

	if (Ring_GenerateData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData7 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData8 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");

	if (Ring_RetriveData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData7 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData8 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");

	// if (Ring_DatatoRRU == NULL)
	// 	rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");


	ret = 0;
	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	//rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL, CALL_MASTER);
	rte_eal_remote_launch(ip_frag_main_loop, NULL,1);
	//rte_eal_remote_launch(l2fwd_launch_one_lcore2, NULL,2);
	rte_eal_remote_launch(ReadData_Loop, NULL,2);
	rte_eal_remote_launch(GenerateData_Loop1, NULL,3);
	rte_eal_remote_launch(GenerateData_Loop2, NULL,4);
	rte_eal_remote_launch(GenerateData_Loop3, NULL,5);
	rte_eal_remote_launch(GenerateData_Loop4, NULL,6);
	rte_eal_remote_launch(GenerateData_Loop5, NULL,7);
	rte_eal_remote_launch(GenerateData_Loop6, NULL,8);
	rte_eal_remote_launch(GenerateData_Loop7, NULL,9);
	rte_eal_remote_launch(GenerateData_Loop8, NULL,10);
	// rte_eal_remote_launch(Data_Retrive_Loop, NULL,11);
	rte_eal_remote_launch(Data_sendto_RRU_loop, NULL,11); 
	/* launch per-lcore init on one lcore */
	//rte_eal_remote_launch(l2fwd_launch_one_lcore, NULL, 1);
	rte_eal_mp_wait_lcore();

	for (portid = 0; portid < nb_ports; portid++) {
		if ((enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");
	return ret;
}
#endif // RUN
