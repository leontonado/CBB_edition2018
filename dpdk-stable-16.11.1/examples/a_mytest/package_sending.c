#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <rte_mbuf.h>
#include <time.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <signal.h>
#include <rte_kni.h>
#include "kni_module.h"
#include "kni_struct.h"
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 8192
#define NB_MBUF                 (8192 * 16)
#define MEMPOOL_CACHE_SZ        PKT_BURST_SZ
#define MAX_PACKET_SZ           2048
#define MBUF_DATA_SZ (MAX_PACKET_SZ + RTE_PKTMBUF_HEADROOM)
#define PKT_BURST_SZ          32
static const char *TransmitData = "TransmitData";
struct rte_ring *Ring_transmitData;
static struct rte_mempool *mbuf_pool;
struct rte_mempool * pktmbuf_pool;
struct kni_interface_stats kni_stats[RTE_MAX_ETHPORTS];
volatile int quit = 0;
int i= 0;
unsigned int count= 0 ;
static rte_atomic32_t kni_stop = RTE_ATOMIC32_INIT(0);

/* Custom handling of signals to handle stats and kni processing */
static void signal_handler(int signum)
{
	/* When we receive a USR1 signal, print stats */
	if (signum == SIGUSR1) {
		print_stats();
	}

	/* When we receive a USR2 signal, reset stats */
	if (signum == SIGUSR2) {
		memset(&kni_stats, 0, sizeof(kni_stats));
		printf("\n**Statistics have been reset**\n");
		return;
	}

	/* When we receive a RTMIN or SIGINT signal, stop kni processing */
	if (signum == SIGRTMIN || signum == SIGINT){
		printf("SIGRTMIN is received, and the KNI processing is "
							"going to stop\n");
		rte_atomic32_inc(&kni_stop);
		return;
        }
}

static int transportData_loop()
{
	while(!quit)
	{
		if(count ==50)
		{
			quit = 1;
			printf("(%u,%d)\n",count,i);
		}
		struct rte_mbuf *Data = NULL;
		char *p_data = NULL;
		Data = rte_pktmbuf_alloc(pktmbuf_pool);
		Data->pkt_len=10;
		p_data = rte_pktmbuf_mtod(Data, char *);
		memset(p_data,i,10);
		printf("%d,\n",*p_data);		
		rte_ring_enqueue(Ring_transmitData,Data);
		//rte_mempool_put(Data->pool, Data);
		count++;
		i=i+2;		
	}
	return 0;
}

static void
kni_ingress(struct kni_port_params *p)
{
	uint8_t i, port_id;
	unsigned num;
	uint32_t nb_kni;
	struct rte_mbuf **pkts_burst;
	void *Data_In_Ring = NULL;

	if (p == NULL)
		return;

	nb_kni = p->nb_kni;
	port_id = p->port_id;
	for (i = 0; i < nb_kni; i++) {
		/* get data from the ring */
		while(rte_ring_dequeue(Ring_transmitData, &Data_In_Ring) <0);
		{
			usleep(100);
		}
		printf("mbuf data is %d\n",*rte_pktmbuf_mtod((struct rte_mbuf *)Data_In_Ring,char *));
		*pkts_burst = (struct rte_mbuf *)Data_In_Ring;
		/* Burst tx to kni */
		num = rte_kni_tx_burst(p->kni[i], pkts_burst, 1);
		kni_stats[port_id].rx_packets += num;

		rte_kni_handle_request(p->kni[i]);
		
		// if (unlikely(num < nb_rx)) {
		// 	/* Free mbufs not tx to kni interface */
		// 	kni_burst_free_mbufs(&pkts_burst[num], nb_rx - num);
		// 	kni_stats[port_id].rx_dropped += nb_rx - num;
		// }
	}
	Data_In_Ring =NULL;
}

/**
 * Interface to dequeue mbufs from tx_q and burst tx
 */
static void kni_egress(struct kni_port_params *p)
{
	uint8_t i, port_id;
	unsigned nb_tx, num;
	uint32_t nb_kni;
	struct rte_mbuf *pkts_burst[PKT_BURST_SZ];

	if (p == NULL)
		return;

	nb_kni = p->nb_kni;
	port_id = p->port_id;
	for (i = 0; i < nb_kni; i++) {
		/* Burst rx from kni */
		num = rte_kni_rx_burst(p->kni[i], pkts_burst, PKT_BURST_SZ);
		if (unlikely(num > PKT_BURST_SZ)) {
			RTE_LOG(ERR, APP, "Error receiving from KNI\n");
			return;
		}
		/* Burst tx to eth */
		nb_tx = rte_eth_tx_burst(port_id, 0, pkts_burst, (uint16_t)num);
		kni_stats[port_id].tx_packets += nb_tx;
		if (unlikely(nb_tx < num)) {
			/* Free mbufs not tx to NIC */
			kni_burst_free_mbufs(&pkts_burst[nb_tx], num - nb_tx);
			kni_stats[port_id].tx_dropped += num - nb_tx;
		}
	}
}

static int kni_lcore_loop(__rte_unused void *arg)
{
	uint8_t i, nb_ports = rte_eth_dev_count();
	int32_t f_stop;
	const unsigned lcore_id = rte_lcore_id();
	enum lcore_rxtx {
		LCORE_NONE,
		LCORE_RX,
		LCORE_TX,
		LCORE_MAX
	};
	enum lcore_rxtx flag = LCORE_NONE;

	for (i = 0; i < nb_ports; i++) {
		if (!kni_port_params_array[i])
			continue;
		if (kni_port_params_array[i]->lcore_rx == (uint8_t)lcore_id) {
			flag = LCORE_RX;
			break;
		} else if (kni_port_params_array[i]->lcore_tx ==
						(uint8_t)lcore_id) {
			flag = LCORE_TX;
			break;
		}
	}

	if (flag == LCORE_RX) {
		RTE_LOG(INFO, APP, "Lcore %u is reading from port %d\n",
					kni_port_params_array[i]->lcore_rx,
					kni_port_params_array[i]->port_id);
		printf("\n");
		while (1) {
			printf("enter kni_ingress\n");
			f_stop = rte_atomic32_read(&kni_stop);
			if (f_stop)
				break;
			kni_ingress(kni_port_params_array[i]);
		}
	} 
	else if (flag == LCORE_TX) {
		RTE_LOG(INFO, APP, "Lcore %u is writing to port %d\n",
					kni_port_params_array[i]->lcore_tx,
					kni_port_params_array[i]->port_id);
		while (1) {
			f_stop = rte_atomic32_read(&kni_stop);
			if (f_stop)
				break;
			kni_egress(kni_port_params_array[i]);
		}
	} 
	else
		RTE_LOG(INFO, APP, "Lcore %u has nothing to do\n", lcore_id);

	return 0;
}

/**
 * Interface to burst rx and enqueue mbufs into rx_q
 */



int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 512;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
	int ret;
	int i;
	uint8_t nb_sys_ports,port;
	/* Associate signal_hanlder function with USR signals */
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	signal(SIGRTMIN, signal_handler);
	signal(SIGINT, signal_handler);
	/* Initialise EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	argc -= ret;
	argv += ret;
	
	ret = parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Could not parse input parameters\n");
	/*create mbuf pool and the ring */
	Ring_transmitData = rte_ring_create(TransmitData , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	if (Ring_transmitData  == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	/* Creates a new mempool in memory to hold the mbufs. */
	// mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
	// 	MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*5, rte_socket_id());
	// if (mbuf_pool == NULL)
	// 	rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	pktmbuf_pool = rte_pktmbuf_pool_create("kni_mbuf_pool", NB_MBUF,
		MEMPOOL_CACHE_SZ, 0, MBUF_DATA_SZ, rte_socket_id());
	if (pktmbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");
	}
	/* Get number of ports found in scan */
	nb_sys_ports = rte_eth_dev_count();
	if (nb_sys_ports == 0)
		rte_exit(EXIT_FAILURE, "No supported Ethernet device found\n");

	/* Check if the configured port ID is valid */
	for (i = 0; i < RTE_MAX_ETHPORTS; i++)
		if (kni_port_params_array[i] && i >= nb_sys_ports)
			rte_exit(EXIT_FAILURE, "Configured invalid "
						"port ID %u\n", i);
	/* Initialize KNI subsystem */
	init_kni();
	for (port = 0; port < nb_sys_ports; port++) {
		/* Skip ports that are not enabled */
		if (!(ports_mask & (1 << port)))
			continue;
		init_port(port);

		if (port >= RTE_MAX_ETHPORTS)
			rte_exit(EXIT_FAILURE, "Can not use more than "
				"%d ports for kni\n", RTE_MAX_ETHPORTS);

		kni_alloc(port);
	}


	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	rte_eal_remote_launch(transportData_loop, NULL,1);
	rte_eal_remote_launch(kni_lcore_loop, NULL, 2);
	rte_eal_remote_launch(kni_lcore_loop,NULL,3);
	rte_eal_mp_wait_lcore();
	/* Release resources */
	for (port = 0; port < nb_sys_ports; port++) {
		if (!(ports_mask & (1 << port)))
			continue;
		kni_free_kni(port);
	}
	for (i = 0; i < RTE_MAX_ETHPORTS; i++)
		if (kni_port_params_array[i]) {
			rte_free(kni_port_params_array[i]);
			kni_port_params_array[i] = NULL;
		}

	return 0;
}