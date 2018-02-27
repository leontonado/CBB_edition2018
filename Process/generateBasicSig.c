#include "../headers/commonStructure.h"
#include "../headers/process.h"
#include "../headers/globalVarINIT.h"
#include <stdlib.h>
#include <memory.h>

void addPilotForSig(complex32* sigAfterMap,complex32* basicSig);

void generateBasicSig(complex32* basicSig, unsigned char* SigInfo, int length){
     unsigned char* sigAfterBCC=(unsigned char*)malloc(48*sizeof(unsigned char));
     MKSUREENMEM(sigAfterBCC);
     memset(sigAfterBCC,0,48*sizeof(unsigned char));
     //BCC
     ccodedot11_init();
     ccodedot11_encode(3, SigInfo, sigAfterBCC, 0);// mode-0 1/2; mode-1 3/4; mode-2 2/3; mode-5 5/6
     //for test
     // FILE* fp=fopen("sigAfterBCC.txt","a+");
     // int i=0;
     // for(i=0;i<48;i++) fprintf(fp,"%d\n",sigAfterBCC[i]);
     // fclose(fp);
     //for test
     //Sig interleaver
     bccInterleaverForSig(sigAfterBCC,48);
     //for test
     /*fp=fopen("sigAfterInterleaver.txt","w");
     for(i=0;i<48;i++) fprintf(fp,"%d\n",sigAfterBCC[i]);
     fclose(fp);*/
     //for test
     //MAP for sig
     complex32* sigAfterMap=(complex32*)malloc(48*sizeof(complex32));
     MKSUREENMEM(sigAfterMap);
     memset(sigAfterMap,0,48*sizeof(complex32));
     MapForSig(sigAfterBCC,sigAfterMap,48);
     //add pilot   
     addPilotForSig(sigAfterMap,basicSig);
     //free data
     free(sigAfterBCC);
     free(sigAfterMap);
}

void addPilotForSig(complex32* sigAfterMap,complex32* basicSig){
     /*length is 64,four 0 in front
     SIG_26 = [1, 1, SigAfterMapper(1:5), P(1), SigAfterMapper(6:18), P(2), SigAfterMapper(19:24),0,...
        SigAfterMapper(25:30), P(3), SigAfterMapper(31:43), P(4), SigAfterMapper(44:end), 1, 1];//size is 57
     */
     int P[4]={1,1,1,-1};
     //basicSig[4].real=1<<dotscale;                                      //we needn't add 4 "1"     (2018/2/7)
     //basicSig[5].real=1<<dotscale;
     memcpy(basicSig+6,sigAfterMap,5*sizeof(complex32));
     basicSig[11].real=P[0]<<dotscale;
     memcpy(basicSig+12,sigAfterMap+5,13*sizeof(complex32));
     basicSig[25].real=P[1]<<dotscale;
     memcpy(basicSig+26,sigAfterMap+18,6*sizeof(complex32));
     memcpy(basicSig+33,sigAfterMap+24,6*sizeof(complex32));
     basicSig[39].real=P[2]<<dotscale;
     memcpy(basicSig+40,sigAfterMap+30,13*sizeof(complex32));
     basicSig[53].real=P[3]<<dotscale;
     memcpy(basicSig+54,sigAfterMap+43,5*sizeof(complex32));
     //basicSig[59].real=1<<dotscale;
     //basicSig[60].real=1<<dotscale;
}

void generateSigA(complex32 *SigA1,complex32 *SigA2,unsigned char *SigABefore)                                         //add function about SigA1&SigA2  (2018/2/7)
{
     unsigned char* sigAafterBCC=(unsigned char*)malloc(96*sizeof(unsigned char));
     unsigned char* BCCinterweaver_1 = (unsigned char*)malloc(48*sizeof(unsigned char));
     unsigned char* BCCinterweaver_2 = (unsigned char*)malloc(48*sizeof(unsigned char));
     complex32* sigA1afterMap=(complex32*)malloc(48*sizeof(complex32));
     complex32* sigA2afterMap=(complex32*)malloc(48*sizeof(complex32));
     memset(sigAafterBCC,0,96*sizeof(unsigned char));
     //BCC
     ccodedot11_init();
     ccodedot11_encode(6, SigABefore, sigAafterBCC, 0);// mode-0 1/2; mode-1 3/4; mode-2 2/3; mode-5 5/6
     //Siga interleaver
     memcpy(BCCinterweaver_1,sigAafterBCC,48);
     memcpy(BCCinterweaver_2,sigAafterBCC+48,48);
     bccInterleaverForSig(BCCinterweaver_1,48);
     bccInterleaverForSig(BCCinterweaver_2,48);
     //MAP for sig1 & sig2
     memset(sigA1afterMap,0,48*sizeof(complex32));
     memset(sigA2afterMap,0,48*sizeof(complex32));  
     MapForSig(BCCinterweaver_1,sigA1afterMap,48);
     MapForSig(BCCinterweaver_2,sigA2afterMap,48);
     //add pilot
     addPilotForSig(sigA1afterMap,SigA1);
     addPilotForSig(sigA2afterMap,SigA2); 
     //free data
     free(sigAafterBCC);
     free(BCCinterweaver_1);
     free(BCCinterweaver_2);
     free(sigA1afterMap);
     free(sigA2afterMap);
}
