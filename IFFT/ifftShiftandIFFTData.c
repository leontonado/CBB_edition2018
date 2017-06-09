#include "../headers/commonStructure.h"
#include "../headers/globalVarINIT.h"
#include "../headers/process.h"
#include <math.h>
#include <stdlib.h>
#include <memory.h>

void ifftShiftandIFFTData(complex32* dataAfterCSD,complex32* dataAfterIFFT){
     int DftSize = SampRate/(Band/4)*subcar;
     float coeff = DftSize / sqrt(N_tone*N_STS);
     //include insert 0 and ifftshift
     complex* pBeforeIFFT=(complex*)malloc(DftSize*sizeof(complex));
     MKSUREENMEM(pBeforeIFFT);
     memset(pBeforeIFFT,0,DftSize*sizeof(complex));
     int i=0;
     int qtr_DftSize = DftSize/4;
     for(i=0;i<qtr_DftSize;i++){
        pBeforeIFFT[i].imag = dataAfterCSD[qtr_DftSize*2+i].imag;
        pBeforeIFFT[i].real = dataAfterCSD[qtr_DftSize*2+i].real;
        pBeforeIFFT[qtr_DftSize*3+i].imag = dataAfterCSD[qtr_DftSize+i].imag;
        pBeforeIFFT[qtr_DftSize*3+i].real = dataAfterCSD[qtr_DftSize+i].real;
     }
     //for test
     /*fp=fopen("stfBeforeIFFT.txt","w");
     for(i=0;i<DftSize;i++) fprintf(fp,"%f %f\n",pBeforeIFFT[i].real,pBeforeIFFT[i].imag);
     fclose(fp);*/
     //for test
     //deal with IFFT
     complex* pAfterIFFTtemp=(complex*)malloc(DftSize*sizeof(complex));
     MKSUREENMEM(pAfterIFFTtemp);
     memset(pAfterIFFTtemp,0,DftSize*sizeof(complex));
     IFFT(pBeforeIFFT,pAfterIFFTtemp,DftSize);
     //power normalization, it may put into addCP
     for(i=0;i<DftSize;i++){
        dataAfterIFFT[i].imag = pAfterIFFTtemp[i].imag*coeff;
        dataAfterIFFT[i].real = pAfterIFFTtemp[i].real*coeff;
     }
     //for test
     /*fp=fopen("stfAfterIFFT.txt","w");
     for(i=0;i<DftSize;i++) fprintf(fp,"%f %f\n",pAfterIFFT[i].real,pAfterIFFT[i].imag);
     fclose(fp);*/
     //for test
     free(pBeforeIFFT);
     free(pAfterIFFTtemp);
}
