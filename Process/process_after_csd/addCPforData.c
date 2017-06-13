#include "../headers/commonStructure.h"
#include "../headers/globalVarINIT.h"
#include "../headers/process.h"
#include <memory.h>

void addCPforData(complex32* pAfterIFFT,complex32* pBeforeAddWin,int N_SYM, int symbol){
    int i;
    float CpDuration=3.2;
    int CpLen = CpDuration*SampRate;
    int DftSize = SampRate/(Band/4)*subcar;
    //data_add_cp = [symbol_data(:, end-CpLen+1:end), symbol_data];

    for(i=0; i<N_STS; i++){
        memcpy(pBeforeAddWin+i*(DftSize+CpLen)*N_SYM+symbol*(DftSize+CpLen),\
         pAfterIFFT+i*DftSize+DftSize-CpLen, CpLen*sizeof(complex32));
        memcpy(pBeforeAddWin+CpLen+i*(DftSize+CpLen)*N_SYM+symbol*(DftSize+CpLen),\
         pAfterIFFT+i*DftSize, DftSize*sizeof(complex32));
    }
/*
     //add cp
     int i=0;
     memcpy(pBeforeAddWin+2,pAfterIFFT+96,32*sizeof(complex32));
     memcpy(pBeforeAddWin+34,pAfterIFFT,128*sizeof(complex32));
     //add Window
     int WT[3]={1200,4096,6992};
     for(i=0;i<3;i++){
        int32 real=pBeforeAddWin[i].real*WT[i]+regForAddWin[NTXindex][i].real*WT[2-i];
        int32 imag=pBeforeAddWin[i].imag*WT[i]+regForAddWin[NTXindex][i].imag*WT[2-i];
        pBeforeAddWin[i].real=real>>dotscale;
        pBeforeAddWin[i].imag=imag>>dotscale;
        regForAddWin[NTXindex][i].real=pBeforeAddWin[160+i].real;
        regForAddWin[NTXindex][i].imag=pBeforeAddWin[160+i].imag;
     }
*/
}
