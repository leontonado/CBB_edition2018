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
     //FILE *fp=fopen("BeforeIFFT.txt","w");
    //for(i=0;i<DftSize;i++) fprintf(fp,"%f %f\n",pBeforeIFFT[i].real,pBeforeIFFT[i].imag);
    // fclose(fp);
     //for test
     //deal with IFFT
     complex* pAfterIFFTtemp=(complex*)malloc(DftSize*sizeof(complex));
     MKSUREENMEM(pAfterIFFTtemp);
     memset(pAfterIFFTtemp,0,DftSize*sizeof(complex));
     IFFT(pBeforeIFFT,pAfterIFFTtemp,DftSize);
    // FILE *z=fopen("IFFT_temp.txt","w");
    // for(i=0;i<DftSize;i++) fprintf(z,"%f %f\n",pAfterIFFTtemp[i].real,pAfterIFFTtemp[i].imag);
    //fclose(z);
     //power normalization, it may put into addCP
     for(i=0;i<DftSize;i++){
        dataAfterIFFT[i].imag = pAfterIFFTtemp[i].imag*coeff;
        dataAfterIFFT[i].real = pAfterIFFTtemp[i].real*coeff;
     }
     //for test
     
     //for test
     free(pBeforeIFFT);
     pBeforeIFFT=NULL;
     free(pAfterIFFTtemp);
     pAfterIFFTtemp=NULL;
}
