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

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

struct rte_ring *send_ring = NULL;
struct rte_ring *recv_ring = NULL;
struct rte_mempool *message_pool =NULL;
volatile int quit = 0;
static const char *_PRI_2_SEC = "PRI_2_SEC";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_MSG_POOL = "MSG_POOL";
static int lcore_recv()
{
	void *msg;
	char *data;
	int n = 0;
	printf("Thread1 starts to receive msg\n");
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
	char  *msg = "hello from thread1";
	char *dest;
	printf("process 1 is beginning to send message\n");
	int n = 100;
	while(1){
		while(data == NULL)
		{
			usleep(5);
			data = rte_pktmbuf_alloc(message_pool);
		}
		dest = rte_pktmbuf_mtod(data,char *);
		memcpy(dest,msg,18);
		//printf("%s\n",dest);
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
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	
	recv_ring = rte_ring_lookup(_PRI_2_SEC);
	send_ring = rte_ring_lookup(_SEC_2_PRI);
	message_pool = rte_mempool_lookup(_MSG_POOL);
	if(recv_ring == NULL){
		printf("recv_ring hasn't been found.\n");
		rte_exit(EXIT_FAILURE,"Problem finding recv_ring.");
	}
	if(send_ring == NULL){
		printf("send_ring hasn't been found.\n");
		rte_exit(EXIT_FAILURE,"Problem finding send_ring.");
	}
	if (message_pool == NULL) {		
		printf("message_pool hasn't been found.\n");
		rte_exit(EXIT_FAILURE,"Problem finding message_pool.");
	}
	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	 rte_eal_remote_launch(lcore_recv, NULL,1);
	rte_eal_remote_launch(lcore_send,NULL,2);
	 rte_eal_mp_wait_lcore();
	return 0;
}
