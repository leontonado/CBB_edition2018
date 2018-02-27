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

#include <time.h> 

#include "allHeaders.h"
//This version runs in the format of frame,which 8 pthread run on 8 individual cores ,but haven't include sending module

//#define RUNMAINDPDK
#ifdef RUNMAINDPDK

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
// #define MEMPOOL_F_SP_PUT         0x0

#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 4095
#define NUM_MBUFS2 1023
#define interval 4
#define Fragment_Num 10
#define User_Num 8
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
const unsigned APEP_LEN_DPDK = 512;

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
struct rte_mempool *mbuf_pool;
struct rte_mempool *mbuf_fragment_pool;
	
volatile int quit = 0;
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
long int Datato707_Loopcount = 0;
long int Data_Distribute_count = 0;
long int Data_Distribute_loop_count = 0;


int N_CBPS, N_SYM, ScrLength, valid_bits;


struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
//static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);


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
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_Datato707, &Data_In_GenerateData) >= 0)
			rte_mempool_put(((struct rte_mbuf *)Data_In_GenerateData)->pool, Data_In_GenerateData);
		else 
		{	
			Datato707_Loopcount++;
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

static int Data_Retrive_Loop() 
{
	struct rte_mbuf *data = NULL;
	void *Data_User_1 ,*Data_User_2,*Data_User_3,*Data_User_4,*Data_User_5,*Data_User_6,*Data_User_7,*Data_User_8;
	complex32 *User_1 ,*User_2,*User_3,*User_4,*User_5,*User_6,*User_7,*User_8,*dest;
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
			Data_Distribute_count++;
			for(i = 0;i <Fragment_Num;i++)
			{
				while(data == NULL) {
				usleep(100);
				data = rte_pktmbuf_alloc(mbuf_fragment_pool);
				}
				data->pkt_len = fragment_length *User_Num*4+8;
				dest = rte_pktmbuf_mtod(data, complex32 *);
				*(char *)dest= User_Num;
				*((char *)dest+1) = 0 ; 		                                                           //        0 represents data
				*((char *)dest+2) = i + 1; 		                                             //        segment sequence num 
				*((char *)dest+3) = Fragment_Num;                                                     //        total segment num
				*((int16*)dest+2) = fragment_length*4;                                //        fragment length
				*((int16*)dest+3) = subcar*N_SYM*N_STS*4;                      //       total length(unit : byte)
				dest+=2;
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
				//fwrite( rte_pktmbuf_mtod(data, complex32 *), sizeof(unsigned char), 4104, tc);
				rte_ring_enqueue(Ring_Datato707,data);

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

		if(Data_Distribute_count >= 5)
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
			printf("Datato707_Loopcount = %ld\n",Datato707_Loopcount );
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
			    usleep(1000);
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



int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 4096;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
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
	return 0;
}
#endif // RUN
