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
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_udp.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <time.h> 

#include "allHeaders.h"
//This version runs in the format of frame,which 8 pthread run on 8 individual cores,and we can transmit data to eth but haven't include the preamble part

#define RUNMAINDPDK
#ifdef RUNMAINDPDK

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 4095
#define NUM_MBUFS2 8192
#define interval 4
#define Fragment_length 1024  //Fragment_length must be multiples of 32(8 user*4 bytes)
#define User_Num 8
/*   basic forwarding limit  */
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define BURST_SIZE 32
#define JUMBO_FRAME_MAXSIZE 9500	

static const char *MBUF_POOL = "MBUF_POOL";
static const char *Mbuf_for_fragment = "MBUF_FOR_FRAGMENT";
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
static const char *Datato707 = "Datato707";

/* create the ring */
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
struct rte_ring *Ring_Datato707;
/*       mempool creation    */
struct rte_mempool *mbuf_pool;
struct rte_mempool *mbuf_fragment_pool;
	
volatile int quit = 0;
const unsigned APEP_LEN_DPDK = 512;
long int GenerateData_Loop1_count = 0;
long int GenerateData_Loop2_count = 0;
long int GenerateData_Loop3_count = 0;
long int GenerateData_Loop4_count = 0;
long int GenerateData_Loop5_count = 0;
long int GenerateData_Loop6_count = 0;
long int GenerateData_Loop7_count = 0;
long int GenerateData_Loop8_count = 0;
long int Ring_full_count = 0;
long int mbuf_full_count = 0;
long int nodataInRing707_count = 0;
long int Data_Distribute_count = 0;
long int Data_Distribute_loop_count = 0;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { 
		.max_rx_pkt_len = ETHER_MAX_LEN ,
		//.jumbo_frame    = 1, /**< Jumbo Frame Support disabled */
		}
};


int N_CBPS, N_SYM, ScrLength, valid_bits;
struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
//static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);

static inline int port_init(uint8_t port, struct rte_mempool *mbuf_pool);

static int ReadData_Loop();
static int GenerateData_Loop1();
static int GenerateData_Loop2();
static int GenerateData_Loop3();
static int GenerateData_Loop4();
static int GenerateData_Loop5();
static int GenerateData_Loop6();
static int GenerateData_Loop7();
static int GenerateData_Loop8();
static int Data_Retrive_Loop();
static int Data_sendto_707_loop();

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
	// FILE *fp = fopen("modulate_data.txt","w+");
	// fwrite(subcar_map_data, sizeof(unsigned char), 5120, fp);
	// fclose(fp);
	return 0;
}	


// static int CSD_encode_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
// {
// 	int i;
// 	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *,0 );
// 	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
// 	for(i=0;i<N_STS;i++){
// 		__Data_CSD_aux(&subcar_map_data, N_SYM, &csd_data,i);
// 	}
// 	return 0;
// }

static int Data_sendto_707_loop()
{
	void *Data_In_GenerateData=NULL;
	const uint8_t nb_ports = rte_eth_dev_count();
	//uint8_t num = 0;
	uint8_t port=0;
	uint16_t nb_tx,buf ;
	struct udp_hdr *u_hdr; 
	struct ipv4_hdr *ip_hdr;
	struct ether_hdr *eth_hdr;
	void * d_addr_bytes;
	struct ether_addr addr;
	struct rte_mbuf*bufs[BURST_SIZE];
	struct rte_mbuf *m;
	rte_eth_macaddr_get(port, &addr);
	//printf("Core %u forwarding packets. \n",rte_lcore_id());

	while (!quit)
	{
		for (port = 0; port < nb_ports; port++) {
				if (rte_ring_dequeue(Ring_Datato707, &Data_In_GenerateData) >= 0)
				{
					m = (struct rte_mbuf *)Data_In_GenerateData;
					//add udp header
					u_hdr = (struct udp_hdr *)
					rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct udp_hdr));
					if(u_hdr == NULL)
						rte_panic("No headroom in mbuf for upd_hdr.\n");
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
					ip_hdr->hdr_checksum = rte_raw_cksum((void *)ip_hdr, 10);
					//add mac header
					eth_hdr = (struct ether_hdr *)
					rte_pktmbuf_prepend(m, (uint16_t)sizeof(struct ether_hdr));
					if (eth_hdr == NULL) {
					rte_panic("No headroom in mbuf.\n");
					}
					m->l2_len = sizeof(struct ether_hdr);
					d_addr_bytes = &eth_hdr->d_addr.addr_bytes[0];
					*((uint64_t *)d_addr_bytes) = 0xC7AFC96ACC4C;
					/* src addr */
					ether_addr_copy(&addr, &eth_hdr->s_addr);
					eth_hdr->ether_type = rte_be_to_cpu_16(ETHER_TYPE_IPv4);
				}			
				else 
				{	
				nodataInRing707_count++;
				continue;
				}
				nb_tx = rte_eth_tx_burst(port, 0,&m, 1);
				Data_Distribute_count++;
			} 			
			if(Data_Distribute_count >=4000)
			{
			quit = 1;
			clock_gettime(CLOCK_REALTIME, &time2);
			time_diff = diff(time1,time2);
			printf("Data_Distribute_count = %ld\n", Data_Distribute_count);
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
			printf("Data_Distribute_loop_count = %ld\n", Data_Distribute_loop_count);
			printf("nodataInRing707_count = %ld\n",nodataInRing707_count );
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

static int Data_Retrive_Loop() 
{
	struct rte_mbuf *data = NULL;
	void *Data_User_1 ,*Data_User_2,*Data_User_3,*Data_User_4,*Data_User_5,*Data_User_6,*Data_User_7,*Data_User_8;
	complex32 *User_1 ,*User_2,*User_3,*User_4,*User_5,*User_6,*User_7,*User_8;
	int i,j,complexnum_PerUser,total_num;
	char *dest;
	int frag_cnt = 1 ;
	unsigned char seq_num = 1;
	complexnum_PerUser = Fragment_length/8/4   ;
	total_num = N_SYM *subcar;
	while(  total_num>complexnum_PerUser)
	{
		total_num -=complexnum_PerUser;		
		frag_cnt++;                                 //calculate how many frags wo need
	}
	//printf("Fragment total num is %d\n",frag_cnt );

	while (!quit)
	{
		if (!check_empty_Ring())
		{			
			while(rte_ring_dequeue(Ring_RetriveData1, &Data_User_1) < 0);
			while(rte_ring_dequeue(Ring_RetriveData2, &Data_User_2) < 0);
			while(rte_ring_dequeue(Ring_RetriveData3, &Data_User_3) < 0);
			while(rte_ring_dequeue(Ring_RetriveData4, &Data_User_4) < 0);
			while(rte_ring_dequeue(Ring_RetriveData5, &Data_User_5) < 0);
			while(rte_ring_dequeue(Ring_RetriveData6, &Data_User_6) < 0);
			while(rte_ring_dequeue(Ring_RetriveData7, &Data_User_7) < 0);
			while(rte_ring_dequeue(Ring_RetriveData8, &Data_User_8) < 0);
			User_1 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_1, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_2 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_2, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_3 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_3, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_4 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_4, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_5 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_5, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_6 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_6, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_7 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_7, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			User_8 = rte_pktmbuf_mtod_offset((struct rte_mbuf *)Data_User_8, complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15 );
			for(i = 0;i <frag_cnt-1;i++)   
			{
				while(data == NULL) {
				usleep(100);
				data = rte_pktmbuf_alloc(mbuf_fragment_pool);
				}
				data->pkt_len = Fragment_length +14;
				data->data_len = Fragment_length+14;
				data->packet_type = (uint32_t)RTE_PTYPE_L3_IPV4; 
				dest = rte_pktmbuf_mtod(data, char *);
				*dest = 0x18  ;                     					                //0x1x represents this is data frame
				if(i==0)
				{
					*(dest+1) = 0x41;
				}	
				else{
					*(dest+1) = 0x80;
				 	*(dest+1) |= i+1;	
				}                                                        				                  //     frag sequence
				*((int16*)dest+1) = 0xffff;                               				     //     store addr
				*((int16*)dest+2) = Fragment_length;                      			     //     data length(unit : byte)
				*(dest+6) = 8 ;                        						     //     represents user num
				*(dest+7) = 0 ; 	  	                                                                                   //     represents data type
				*(dest+8) = i+1; 	                                                                                   //     sequence of this fragment
				*(dest+9) = frag_cnt;	                                                                                   //     fragment total num 
				*((int16*)dest+5) = Fragment_length;                                                             //     fragment length
				*((int16*)dest+6) = N_SYM *subcar *4; 		                                            //     total length of a frame
				dest+=14;
				for(j=0;j<complexnum_PerUser;j++)
				{
					*((complex32*)dest+User_Num*j) = *(User_1+j+complexnum_PerUser*i);
					*((complex32*)dest+1+User_Num*j) = *(User_2+j+complexnum_PerUser*i);
					*((complex32*)dest+2+User_Num*j) = *(User_3+j+complexnum_PerUser*i);
					*((complex32*)dest+3+User_Num*j) = *(User_4+j+complexnum_PerUser*i);
					*((complex32*)dest+4+User_Num*j) = *(User_5+j+complexnum_PerUser*i);
					*((complex32*)dest+5+User_Num*j) = *(User_6+j+complexnum_PerUser*i);
					*((complex32*)dest+6+User_Num*j) = *(User_7+j+complexnum_PerUser*i);
					*((complex32*)dest+7+User_Num*j) = *(User_8+j+complexnum_PerUser*i);
				} 
				rte_ring_enqueue(Ring_Datato707,data);
				data =NULL;
			}
			//              last fragment
			while(data == NULL) {
				usleep(100);
				data = rte_pktmbuf_alloc(mbuf_fragment_pool);
				}
			data->pkt_len = total_num*8*4 +14;
			data->data_len = total_num*8*4+14;
			data->packet_type = (uint32_t)RTE_PTYPE_L3_IPV4; 
			dest = rte_pktmbuf_mtod(data, char *);
			*dest = 0x18  ;                     					       //0x1x represents this is data frame
			*(dest+1) = 0xc0;
			*(dest+1)|=frag_cnt;	                                                                                   //       lasts frag sequence
			*((int16*)dest+1) = 0xffff;                               				     //       store addr
			*((int16*)dest+2) = total_num*8*4;                      			     //       data length(unit : byte)
			*(dest+6) = 8 ;                                                                                                   //       represents user num
			*(dest+7) = 0 ; 	  	                                                                                   //       represents data type
			*(dest+8) = frag_cnt; 	                                                                                   //       sequence of this fragment
			*(dest+9) = frag_cnt;	                                                                                   //       fragment total num 
			*((int16*)dest+5) = total_num *4;                                                                    //       fragment length
			*((int16*)dest+6) = N_SYM *subcar ; 		                                             //       total length of a frame
			dest+=14;
			for(j=0;j<total_num;j++)
			{
				*((complex32*)dest+User_Num*j) = *(User_1+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+1+User_Num*j) = *(User_2+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+2+User_Num*j) = *(User_3+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+3+User_Num*j) = *(User_4+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+4+User_Num*j) = *(User_5+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+5+User_Num*j) = *(User_6+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+6+User_Num*j) = *(User_7+j+complexnum_PerUser*(frag_cnt -1));
				*((complex32*)dest+7+User_Num*j) = *(User_8+j+complexnum_PerUser*(frag_cnt -1));
			} 
			rte_ring_enqueue(Ring_Datato707,data);
			data =NULL;
			rte_mempool_put(((struct rte_mbuf *)Data_User_1)->pool, Data_User_1);
			rte_mempool_put(((struct rte_mbuf *)Data_User_2)->pool, Data_User_2);
			rte_mempool_put(((struct rte_mbuf *)Data_User_3)->pool, Data_User_3);
			rte_mempool_put(((struct rte_mbuf *)Data_User_4)->pool, Data_User_4);
			rte_mempool_put(((struct rte_mbuf *)Data_User_5)->pool, Data_User_5);
			rte_mempool_put(((struct rte_mbuf *)Data_User_6)->pool, Data_User_6);
			rte_mempool_put(((struct rte_mbuf *)Data_User_7)->pool, Data_User_7);
			rte_mempool_put(((struct rte_mbuf *)Data_User_8)->pool, Data_User_8);
		}
		else 
		{
			Data_Distribute_loop_count++;
			usleep(1000);
			continue;
		}
		
	}
	return 0 ;
}

static int GenerateData_Loop1() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop2() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop3() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop4() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop5() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop6() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop7() 
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
			//usleep(1000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop8() 
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
			//usleep(1000);
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

static int ReadData_Loop() 
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
			    usleep(100);
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

static inline int port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count())
		return -1;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(port);

	return 0;
}


int main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 4096;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
	uint8_t portid;
	unsigned nb_ports;
	int ret;
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
	//unsigned lcore_id;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	nb_ports = rte_eth_dev_count();

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

	Ring_Datato707 = rte_ring_create(Datato707, 1024, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);

	if (Ring_GenerateData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting  Ring_GenerateData1\n");
	if (Ring_GenerateData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData2\n");
	if (Ring_GenerateData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData3\n");
	if (Ring_GenerateData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData4\n");
	if (Ring_GenerateData5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData5\n");
	if (Ring_GenerateData6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData6\n");
	if (Ring_GenerateData7 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData7\n");
	if (Ring_GenerateData8 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_GenerateData8\n");

	if (Ring_RetriveData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData1\n");
	if (Ring_RetriveData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData2\n");
	if (Ring_RetriveData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData3\n");
	if (Ring_RetriveData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData4\n");
	if (Ring_RetriveData5 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData5\n");
	if (Ring_RetriveData6 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData6\n");
	if (Ring_RetriveData7 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData7\n");
	if (Ring_RetriveData8 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_RetriveData8\n");

	if (Ring_Datato707 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_Datato707\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create(MBUF_POOL, NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*30, rte_socket_id());

	mbuf_fragment_pool = rte_pktmbuf_pool_create(Mbuf_for_fragment,NUM_MBUFS2,MBUF_CACHE_SIZE,
		0,RTE_MBUF_DEFAULT_BUF_SIZE*18,rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	if (mbuf_fragment_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool for fragmentation:%s\n",rte_strerror(rte_errno));
	/* Initialize all ports. */
	for (portid = 0; portid < nb_ports; portid++)
		if (port_init(portid, mbuf_fragment_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
					portid);
	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	rte_eal_remote_launch(ReadData_Loop, NULL,1);
	rte_eal_remote_launch(GenerateData_Loop1, NULL,2);
	rte_eal_remote_launch(GenerateData_Loop2, NULL,3);
	rte_eal_remote_launch(GenerateData_Loop3, NULL,4);
	rte_eal_remote_launch(GenerateData_Loop4, NULL,5);
	rte_eal_remote_launch(GenerateData_Loop5, NULL,6);
	rte_eal_remote_launch(GenerateData_Loop6, NULL,7);
	rte_eal_remote_launch(GenerateData_Loop7, NULL,8);
	rte_eal_remote_launch(GenerateData_Loop8, NULL,9);
	rte_eal_remote_launch(Data_Retrive_Loop, NULL,10);
	rte_eal_remote_launch(Data_sendto_707_loop, NULL,11); 
	rte_eal_mp_wait_lcore();
	for (portid = 0; portid < nb_ports; portid++) {
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");
	return 0;
}
#endif // RUN
