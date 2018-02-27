#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../allHeaders.h"
#include <math.h>
#include <string.h>
complex32 *pSTF ;
complex32 *pLTF ;
complex32 *pL_SIG; 
complex32 *pR_SIG;
complex32 *SIGA1;
complex32 *SIGA2;
complex32 *pHELTF;

int numberOfHeLTF();
void generatePreambleAndHeLTF_csd(void)
{
    int n_ltf=0;
    int N_SYM=0;
    //set the information of sig
    unsigned char SigInfo[3];
    setSigInfo(SigInfo,3);
    int i=0;
    for(i=0;i<N_TX;i++){
        STF[i]=(complex32*)malloc(64*sizeof(complex32));
        LTF[i]=(complex32*)malloc(64*sizeof(complex32));
        Sig[i]=(complex32*)malloc(64*sizeof(complex32));
        MKSUREENMEM(STF[i]);
        MKSUREENMEM(LTF[i]);
        MKSUREENMEM(Sig[i]);
        memset(STF[i],0,64*sizeof(complex32));
        memset(LTF[i],0,64*sizeof(complex32));
        memset(Sig[i],0,64*sizeof(complex32));
        complex32* pStfStart = STF[i];
        complex32* pLtfStart = LTF[i];
        complex32* pSigStart = Sig[i];
        generatePreamble_csd(pStfStart,pLtfStart,SigInfo,pSigStart,i);
 }
    //HE LTF
    //length of HeLTF
    n_ltf=numberOfHeLTF();
    for(i=0;i<N_TX;i++)
    {
        heLTF[i]=(complex32*)malloc(n_ltf*256*sizeof(complex32));
        complex32* pHeLTFStart=heLTF[i];
        generateHeLTF_csd(pHeLTFStart,i,n_ltf);
    }
    
    //#define testforinit 
    #ifdef testforinit
    //save STF data
    FILE *fp=fopen("STF_csd.txt", "w");
    for(i=0;i<N_TX;i++) printStreamToFile(STF[i],64,fp);
    fclose(fp);
    //save LTF data
    fp=fopen("LTF_csd.txt", "w");
    for(i=0;i<N_TX;i++) printStreamToFile(LTF[i],64,fp);
    fclose(fp);
    //save Sig data
    fp=fopen("Sig_csd.txt", "w");
    for(i=0;i<N_TX;i++) printStreamToFile(Sig[i],64,fp);
    fclose(fp);
    //save HeLTF data
    fp=fopen("HeLTF_csd.txt", "w");
    for(i=0;i<N_TX;i++) printStreamToFile(heLTF[i],n_ltf*256,fp);
    fclose(fp);
    #endif
    
}

//Because of deleting csd part , we rewrite preamble part.The following module includes the generation of STF,LTF,L-sig ,R-sig,sigA1 and sigA2 (2018/2/7)
void generatePreamble_withSigA1()
{
        int n_ltf;
        pSTF = (complex32 *)malloc(64*sizeof(complex32));
        pLTF = (complex32 *)malloc(64*sizeof(complex32));
        pL_SIG = (complex32 *)malloc(64*sizeof(complex32));
        pR_SIG = (complex32 *)malloc(64*sizeof(complex32));
        SIGA1 = (complex32 *)malloc(64*sizeof(complex32));
        SIGA2  = (complex32 *)malloc(64*sizeof(complex32));
        unsigned char SigInfo[3];
        unsigned char SigABefore[6]={0};
        setSigInfo(SigInfo,3);
        // FILE* fp=fopen("sigAfterBCC.txt","w+");
        // for(i=0;i<3;i++)fprintf(fp, "siginfo[%d]=%d\n",i, SigInfo[i]);
        // fclose(fp);
        MKSUREENMEM(pSTF);
        MKSUREENMEM(pLTF);
        MKSUREENMEM(pL_SIG);
        MKSUREENMEM(pR_SIG);
        MKSUREENMEM(SIGA1);
        MKSUREENMEM(SIGA2);
        memset(pSTF, 0, sizeof(complex32)*64);
        memset(pLTF, 0, sizeof(complex32)*64);
        memset(pL_SIG, 0, sizeof(complex32)*64);
        memset(pR_SIG, 0, sizeof(complex32)*64);
        memcpy(pSTF+6,basicSTF,53*4);
        memcpy(pLTF+6,basicLTF,53*4);
        generateBasicSig(pL_SIG,SigInfo,64); 
        memcpy(pR_SIG,pL_SIG,64*4);                                         //R_Sig data equals to L_Sig
        generateSigA(SIGA1, SIGA2, SigABefore);
         // n_ltf=numberOfHeLTF();
         pHELTF = (complex32 *)malloc(256*sizeof(complex32));
         generateHeLTF_csd(pHELTF,0,1);
}

int numberOfData(){
    int N_service = 16;
    int N_tail = 6;
    unsigned char rate_type;
    int N_BPSCS, N_DBPS,N_CBPS, N_ES, N_SYM;
    mcs_table_for_20M(&rate_type, &N_BPSCS, &N_DBPS, &N_CBPS, &N_ES);
    N_SYM = ceil(((double)(8*APEP_LEN + N_service + N_tail*N_ES) / (double)N_DBPS));
    return N_SYM;
}



void printStreamToFile(complex32* pData, int length, FILE* fp){
    int n=length;
    while(n--){
        fprintf(fp,"(%d,%d)\n",pData->real,pData->imag);
         //fprintf(fp,"%d%d",pData->real,pData->imag);
        ++pData;
    }
}




int numberOfHeLTF()
{
       int ntx=N_TX;
       if(!(ntx&(ntx-1))) return ntx;
       else return ntx+1;
}

//#define debug
#ifdef debug
int main()
{
        generatePreamble_withSigA1();
        // FILE *fp=fopen("STF.txt", "w+");
        //         printStreamToFile(pSTF,64,fp);
        // fclose(fp);
        // //save LTF data
        // fp=fopen("LTF.txt", "w+");
        //         printStreamToFile(pLTF,64,fp);
        // fclose(fp);
        // //save L-Sig data
        // fp=fopen("L-Sig.txt", "w+");
        //         printStreamToFile(pL_SIG,64,fp);
        // fclose(fp);
        // fp=fopen("SigA2.txt","w+");
        //         printStreamToFile(SIGA2,64,fp);
        // fclose(fp);
        return 0;
}
#endif