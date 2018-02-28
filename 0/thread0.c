#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>
#include <rte_mbuf.h>
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

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
#define NUM_MBUFS 1024
#define MBUF_CACHE_SIZE 128
static const char *_MSG_POOL = "MSG_POOL";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_PRI_2_SEC = "PRI_2_SEC";
const unsigned string_size = 64;

struct rte_ring *send_ring,*recv_ring;
struct rte_mempool *message_pool;
volatile int quit = 0;
static int lcore_recv()
{
	void *msg;
	char *data;
	int n = 0;
	printf(" Process0 Starting to receive msg\n");
	while (1){
		
		if (rte_ring_dequeue(recv_ring, &msg) < 0){
			usleep(5);
			printf("wait~\n");
			continue;
		}
		data = rte_pktmbuf_mtod((struct rte_mbuf*)msg,char *);
		n++;
		printf("Received data'%s',%d\n", data,n);
		rte_mempool_put(message_pool, msg);
	}
	
	return 0;
}
static int lcore_send()
{
	struct rte_mbuf *data = NULL;
	char  *msg = "hello from thread0";
	char *dest;
	printf("process 0 is beginning to send message\n");
	int n = 0;
	while(1){
		while(data == NULL)
		{
			usleep(5);
			data = rte_pktmbuf_alloc(message_pool);
		}
		dest = rte_pktmbuf_mtod(data,char *);
		memcpy(dest,msg,18);
		//printf("%s,%d\n",dest,n++);
		if (rte_ring_enqueue(send_ring, data) < 0) {
		printf("Failed to send message - message discarded\n");
		rte_mempool_put(message_pool, data);		
		}
		data =NULL;
		usleep(5);
	}
	return 0;
}


int main(int argc, char **argv)
{
	int ret;
	const unsigned ring_size = 256;
	const unsigned flags = 0;
	unsigned lcore_id;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	send_ring = rte_ring_create(_PRI_2_SEC, ring_size, rte_socket_id(), flags);
	recv_ring = rte_ring_create(_SEC_2_PRI, ring_size, rte_socket_id(), flags);
	message_pool = rte_pktmbuf_pool_create(_MSG_POOL, NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (send_ring == NULL)
		rte_exit(EXIT_FAILURE, "Problem creating send_ring\n");
	if(recv_ring == NULL)
		rte_exit(EXIT_FAILURE,"Problem creating recv_ring\n");
	if (message_pool == NULL)
		rte_exit(EXIT_FAILURE, "Problem creating message_pool\n");
	RTE_LOG(INFO, APP, "Finished Process Init.\n");
		rte_eal_remote_launch(lcore_send, NULL, 1);
		rte_eal_remote_launch(lcore_recv,NULL,2);
	rte_eal_mp_wait_lcore();
	return 0;
}
