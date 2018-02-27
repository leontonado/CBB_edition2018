#include "../headers/process.h"

void setSigInfo(unsigned char* SigInfo, int length){
     //the SigInfo contains the data below
     /*Rate = [1 1 0 1];
     Reserved = 0;
     Length = [0 0 1 0 0 0 0 0 0 0 0 0];
     Parity = 0;
     Tail = [0 0 0 0 0 0];
     SigInfo = [Rate, Reserved, Length, Parity, Tail];
     SigInfo[0]=0x8B;
     SigInfo[1]=0x00;
     SigInfo[2]=0x00;*/
     
     //new Siginfo referring to Yuan's matlab code
     /*Rate = [0 0 0 0];
     Reserved = 0;
     Frame_type = [0 1 0];
     datalength = [0 0 0 0 0 0 0 0 0];
     Length = [Frame_type datalength];
     Parity = 0;    //奇偶校验
     Tail = [0 0 0 0 0 0];
     SigInfo = [Rate, Reserved, Length, Parity, Tail];*/
     SigInfo[0]=0x40;
     SigInfo[1]=0x00;
     SigInfo[2]=0x00;
}
