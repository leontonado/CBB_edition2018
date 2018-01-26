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

#include "allHeaders.h"
//This version runs in the format of frame,which 8 pthread run on 8 individual cores,but without CSD module

#define RUNMAINDPDK
#ifdef RUNMAINDPDK

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
// #define MEMPOOL_F_SP_PUT         0x0

#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 4095
#define MBUFS    4096*16
#define interval 4
#define User_Num 8
#define PKT_BURST_SZ          32
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
static const char *Send2kernel = "Send2kernel";

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

struct rte_ring *Ring_Send2kernel;

struct rte_mempool *mbuf_pool;
struct rte_mempool *pktmbuf_pool;	
volatile int quit = 0;

long int ReadData_count = 0;
long int Data_Distribute_count = 0;
long int GenerateData_Loop1_count = 0;
long int GenerateData_Loop2_count = 0;
long int GenerateData_Loop3_count = 0;
long int GenerateData_Loop4_count = 0;
long int GenerateData_Loop5_count = 0;
long int GenerateData_Loop6_count = 0;
long int GenerateData_Loop7_count = 0;
long int Data_Distribute_loop_count = 0;
long int ReadData_Loop_count = 0;
long int NotGetData_count=0;

long int AllRingfull  = 0; 

int N_CBPS, N_SYM, ScrLength, valid_bits;
//fragmentation &kni module
int Fragment_Num =10;
struct kni_interface_stats kni_stats[RTE_MAX_ETHPORTS];
//static rte_atomic32_t kni_stop = RTE_ATOMIC32_INIT(0);

struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);
static int IFFTAndaddWindow_dpdk(__attribute__((unused)) struct rte_mbuf *Data_In);

static int ReadData_Loop();
static int GenerateData_Loop1();
static int GenerateData_Loop2();
static int GenerateData_Loop3();
static int GenerateData_Loop4();
static int GenerateData_Loop5();
static int GenerateData_Loop6();
static int GenerateData_Loop7();
static int Data_Distribute_Loop() ;
static void signal_handler(int signum);
static void kni_ingress(struct kni_port_params *p);
static void kni_egress(struct kni_port_params *p);
static int kni_lcore_loop(__rte_unused void *arg);
struct timespec time1,time2,time_diff;	/** < Test the running time. >*/

struct timespec diff(struct timespec start, struct timespec end)
{
    struct  timespec temp;

     if ((end.tv_nsec-start.tv_nsec)<0) {
         temp.tv_sec = end.tv_sec-start.tv_sec-1;
         temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
     } else {
         temp.tv_sec = end.tv_sec-start.tv_sec;
         temp.tv_nsec = end.tv_nsec-start.tv_nsec;
     }
     return temp;
}

static int InitData(unsigned char** p_databits)
{
	int i=0;
	FILE *fp=fopen("send_din_dec.txt","rt");
	unsigned char *databits=(unsigned char*)malloc(APEP_LEN_DPDK*sizeof(unsigned char));
	*p_databits = databits;
	if(databits == NULL){
		printf("error");
		return 0;
	}
	unsigned int datatmp=0;
	for(i;i<APEP_LEN_DPDK;i++){
	    fscanf(fp,"%ud",&datatmp);
	    databits[i]=datatmp&0x000000FF;
	}
	//memcpy(rte_pktmbuf_mtod(Data,unsigned char *), databits, APEP_LEN_DPDK);
	fclose(fp);
	return 0;
}

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in) 
{
	//printf("Data->buflen = %d\n",Data_out->buf_len);
	//printf("ReadData_count = %d\n", ReadData_count++);
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
	//printf("GenDataAndScramble_DPDK_count = %ld\n", GenDataAndScramble_DPDK_count++);
	
	unsigned char *databits = rte_pktmbuf_mtod(Data_In, unsigned char *);
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	GenDataAndScramble(data_scramble, ScrLength, databits, valid_bits);	

	return 0;
}

static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("BCC_encoder_DPDK_count = %ld\n", BCC_encoder_DPDK_count++);

	int CodeLength = N_SYM*N_CBPS/N_STS;
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	BCC_encoder_OPT(data_scramble, ScrLength, N_SYM, &BCCencodeout, CodeLength);

	return 0;
}

static int modulate_DPDK(__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("modulate_DPDK_count = %ld\n", modulate_DPDK_count++);

	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*15);

	modulate_mapping(BCCencodeout,&subcar_map_data);
	// FILE *k=fopen("modulate_data.txt","w");
	// printStreamToFile_float(subcar_map_data,1280,k);
	// fclose(k);
	return 0;
}	

// static int CSD_encode_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
// {
// 	int i;
// 	//printf("CSD_encode_DPDK_count = %ld\n", CSD_encode_DPDK_count++);
// 	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *,0 );
// 	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*15);
// 	//Data_CSD(&subcar_map_data, N_SYM, &csd_data);
	
// 	for(i=0;i<N_STS;i++){
// 		__Data_CSD_aux(&subcar_map_data, N_SYM, &csd_data,i);
// 	}
// 	return 0;
// }

// static int IFFTAndaddWindow_dpdk(__attribute__((unused)) struct rte_mbuf *Data_In)
// {
// 	int i;
// 	//IFFTAndaddWindow_dpdk_count++;
// 	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *,0 );
// 	complex32 *IFFT_data = rte_pktmbuf_mtod_offset(Data_In,complex32 *,RTE_MBUF_DEFAULT_BUF_SIZE*15);
// 	csd_data_IDFT(csd_data,IFFT_data,N_SYM);
// 	FILE *k=fopen("IFFT_data.txt","w");
// 	printStreamToFile_float(IFFT_data,5120,k);
// 	fclose(k);
// 	return 0;
// }

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

static int Data_Distribute_Loop() 
{
	struct rte_mbuf *data = NULL;
	void *Data_User_1,*Data_User_2,*Data_User_3,*Data_User_4,*Data_User_5,*Data_User_6,*Data_User_7,*Data_User_8;
	complex32 *User_1,*User_2,*User_3,*User_4,*User_5,*User_6,*User_7,*User_8,*dest;
	int i,j,fragment_length;
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
			fragment_length = subcar*N_SYM*N_STS/Fragment_Num;
			printf("fragment_length = %d\n",fragment_length );

			for(i = 0;i <Fragment_Num;i++)
			{
				while(data == NULL) {
				usleep(100);
				data = rte_pktmbuf_alloc(pktmbuf_pool);
				}
				dest = rte_pktmbuf_mtod(data, complex32 *);
				*(char *)dest= User_Num;
				*(char *)(dest+1) = 0 ; 		                                                           //        0 represents data
				*(char *)(dest+2) = i + 1; 		                                             //        segment sequence num 
				*(char *)(dest+3) = Fragment_Num;                                                    //        total segment num
				*(int16*)(dest+4) = fragment_length*4;                                //        fragment length
				*(int16*)(dest+6) = subcar*N_SYM*N_STS*4;                      //       total length(unit : byte)
				dest = rte_pktmbuf_mtod_offset(data, complex32 *, 2);
				for(j=0;j<fragment_length;j++)
				{
					*(dest+User_Num*j) = *(User_1+j+fragment_length*i);
					*(dest+1+User_Num*j) = *(User_2+j+fragment_length*i);
					*(dest+2+User_Num*j) = *(User_3+j+fragment_length*i);
					*(dest+3+User_Num*j) = *(User_4+j+fragment_length*i);
					*(dest+4+User_Num*j) = *(User_5+j+fragment_length*i);
					*(dest+5+User_Num*j) = *(User_6+j+fragment_length*i);
					*(dest+6+User_Num*j) = *(User_7+j+fragment_length*i);
					*(dest+7+User_Num*j) = *(User_8+j+fragment_length*i);
				}
				 //----------transmit to kernel module--------//
				data->pkt_len = fragment_length *User_Num*4;
				rte_ring_enqueue(Ring_Send2kernel,data);
				Data_Distribute_count++;
				data =NULL;

			}
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
		if(Data_Distribute_count >= 100)
		{
			quit = 1;
			clock_gettime(CLOCK_REALTIME, &time2);
			time_diff = diff(time1,time2);
			printf("Data_Distribute_count = %ld\n", Data_Distribute_count);
			printf("Start time # %.24s %ld Nanoseconds \n",ctime(&time1.tv_sec), time1.tv_nsec);
			printf("Stop time # %.24s %ld Nanoseconds \n",ctime(&time2.tv_sec), time2.tv_nsec);
			printf("Running time # %ld.%ld Seconds \n",time_diff.tv_sec, time_diff.tv_nsec);
			printf("ReadData_Loop_count = %ld\n", ReadData_Loop_count);
			printf("NotGetData_count=%ld\n",NotGetData_count);
			printf("GenerateData_Loop1_count = %ld\n", GenerateData_Loop1_count);
			printf("GenerateData_Loop2_count = %ld\n", GenerateData_Loop2_count);
			printf("GenerateData_Loop3_count = %ld\n", GenerateData_Loop3_count);
			printf("GenerateData_Loop4_count = %ld\n", GenerateData_Loop4_count);
			printf("GenerateData_Loop5_count = %ld\n", GenerateData_Loop5_count);
            			printf("GenerateData_Loop6_count = %ld\n", GenerateData_Loop6_count);
           			printf("GenerateData_Loop7_count = %ld\n", GenerateData_Loop7_count);
			printf("Data_Distribute_loop_count = %ld\n", Data_Distribute_loop_count);
			printf("Fullring= %ld\n\n",AllRingfull);
		}


	}
	return 0;
}

static int GenerateData_Loop1() 
{
	void *Data_In_GenerateData=NULL;
	printf("a\n");
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData1, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			//CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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

static int GenerateData_Loop2() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData2, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			//CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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

static int GenerateData_Loop3() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData3, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			//CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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

static int GenerateData_Loop4() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData4, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			//CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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

static int GenerateData_Loop5() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData5, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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

static int GenerateData_Loop6() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData6, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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
static int GenerateData_Loop7() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData7, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			// CSD_encode_DPDK(Data_In_GenerateData);
			//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
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
static int FindEmptyRing()
{
	if(rte_ring_full(Ring_GenerateData1)==0)
	{
		return 1;
	}
	else if(rte_ring_full(Ring_GenerateData2)==0)
	{
		return 2;
	}
	else if(rte_ring_full(Ring_GenerateData3)==0)
	{
		return 3;
	}
	else if(rte_ring_full(Ring_GenerateData4)==0)
	{
		return 4;
	}
	else if(rte_ring_full(Ring_GenerateData5)==0)
	{
		return 5;
	}
	else if(rte_ring_full(Ring_GenerateData6)==0)
	{
		return 6;
	}
	else if(rte_ring_full(Ring_GenerateData7)==0)
	{
		return 7;
	}
	else if(rte_ring_full(Ring_GenerateData8)==0)
	{
		return 8;
	}
	else 
	{
		return 0;
	} 
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
	void *Data_In_GenerateData=NULL;
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
			   if(dis_count == interval*8){
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
		   else if (rte_ring_dequeue(Ring_GenerateData8, &Data_In_GenerateData) >= 0){
		   		ReadData_Loop_count++;
			    GenDataAndScramble_DPDK(Data_In_GenerateData);
			    BCC_encoder_DPDK(Data_In_GenerateData);
			    modulate_DPDK(Data_In_GenerateData);
			    //CSD_encode_DPDK(Data_In_GenerateData);
			  	//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
			    rte_ring_enqueue(Ring_RetriveData8, Data_In_GenerateData);
		    }
		    else {
			 
			    //usleep(100000);
			    continue;
		    }
		    
	    //else
	    //{
	    //	
	   // }
	    }
	    else {
	    	AllRingfull++;
	    	if (rte_ring_dequeue(Ring_GenerateData8, &Data_In_GenerateData) >= 0){
			    GenDataAndScramble_DPDK(Data_In_GenerateData);
			    BCC_encoder_DPDK(Data_In_GenerateData);
			    modulate_DPDK(Data_In_GenerateData);
			    //CSD_encode_DPDK(Data_In_GenerateData);
			  	//IFFTAndaddWindow_dpdk(Data_In_GenerateData);
			    rte_ring_enqueue(Ring_RetriveData8, Data_In_GenerateData);
	    	}
	    	else{
	    		NotGetData_count++;
	    	}
	    }
    }
	return 0;
}

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
		printf("SIGRTMIN is received, and the dpdk processing is going to stop\n");
		quit =1;
		return;
        }
}

static void kni_ingress(struct kni_port_params *p)
{
	uint8_t i, port_id;
	unsigned num;
	uint32_t nb_kni;
	struct rte_mbuf *pkts_burst;
	void *Data_In_Ring = NULL;

	if (p == NULL)
		return;

	nb_kni = p->nb_kni;
	port_id = p->port_id;
	for (i = 0; i < nb_kni; i++) {
		/* get data from the ring */
		while(rte_ring_dequeue(Ring_Send2kernel, &Data_In_Ring) <0);
		{
			usleep(100);
		}
		//printf("Second is %d\n",*(rte_pktmbuf_mtod((struct rte_mbuf *)Data_In_Ring,char *)+1));
		pkts_burst = (struct rte_mbuf *)Data_In_Ring;
		/* Burst tx to kni */
		num = rte_kni_tx_burst(p->kni[i], &pkts_burst, 1);
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
		while (!quit) {
			//printf("enter kni_ingress\n");
			// f_stop = rte_atomic32_read(&kni_stop);
			// if (f_stop)
			// 	break;
			kni_ingress(kni_port_params_array[i]);
		}
	} 
	else if (flag == LCORE_TX) {
		RTE_LOG(INFO, APP, "Lcore %u is writing to port %d\n",
					kni_port_params_array[i]->lcore_tx,
					kni_port_params_array[i]->port_id);
		while (!quit) {
			// f_stop = rte_atomic32_read(&kni_stop);
			// if (f_stop)
			// 	break;
			kni_egress(kni_port_params_array[i]);
		}
	} 
	else
		RTE_LOG(INFO, APP, "Lcore %u has nothing to do\n", lcore_id);

	return 0;
}

int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 512;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
	int ret,i;
	uint8_t nb_sys_ports,port;
	/* Associate signal_hanlder function with USR signals */
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	signal(SIGRTMIN, signal_handler);
	signal(SIGINT, signal_handler);

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
   	// printf("N_CBPS=%d,N_SYM=%d,ScrLength=%d",N_CBPS,N_SYM,ScrLength);
   	// 运行一次得到生成导频的分流交织表
	initial_streamwave_table(N_SYM);
	init_mapping_table();
	///////////////////////////////////////////////////////////////////////////////////
	//unsigned lcore_id;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	argc -= ret;
	argv += ret;
	ret = parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Could not parse input parameters\n");
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
	Ring_Send2kernel = rte_ring_create(Send2kernel,ring_size,rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
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
	if(Ring_Send2kernel == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting Ring_Send2kernel\n");
	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*30, rte_socket_id());
	pktmbuf_pool = rte_pktmbuf_pool_create("pktmbuf_pool", MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*10, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	if (pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/*                        kni module                      */
	nb_sys_ports = rte_eth_dev_count();
	//printf("%d\n",nb_sys_ports );
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
	/*                          kni  module                 */   

	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	rte_eal_remote_launch(kni_lcore_loop, NULL, 1);
	rte_eal_remote_launch(kni_lcore_loop,NULL,2);
	rte_eal_remote_launch(ReadData_Loop, NULL,3);
	rte_eal_remote_launch(GenerateData_Loop1, NULL,4);
	rte_eal_remote_launch(GenerateData_Loop2, NULL,5);
	rte_eal_remote_launch(GenerateData_Loop3, NULL,6);
	rte_eal_remote_launch(GenerateData_Loop4, NULL,7);
	rte_eal_remote_launch(GenerateData_Loop5, NULL,8);
	rte_eal_remote_launch(GenerateData_Loop6, NULL,9);
	rte_eal_remote_launch(GenerateData_Loop7, NULL,10);
	rte_eal_remote_launch(Data_Distribute_Loop, NULL,11);

	rte_eal_mp_wait_lcore();
	/* Release resources */
	for (port = 0; port < nb_sys_ports; port++) {
		if (!(ports_mask & (1 << port)))
			continue;
		kni_free_kni(port);
	}
	for (i = 0; i < RTE_MAX_ETHPORTS; i++)
	{
		if (kni_port_params_array[i]) {
			rte_free(kni_port_params_array[i]);
			kni_port_params_array[i] = NULL;
		}
	}
	return 0;
}
#endif // RUN