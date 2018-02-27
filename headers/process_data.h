#ifndef PROCESS_DATA
#define PROCESS_DATA
#include "../allHeaders.h"
#include "commonStructure.h"

#define twice(N) (1<<(N))

extern void He_LTF_IDFT(complex32 **X_VHTLTF_CSD, complex32 **IDFT_X_VHTLTF);
extern void addCPforHELTF(complex32 **X_VHTLTF, complex32 **IDFT_X_VHTLTF);

//�������ݵ�������
extern void GenerateData(unsigned char *databits, complex32 **csd_data);

//MCS�����
extern void mcs_table_for_20M(unsigned char *rate_type, int *N_BPSCS, int *N_DBPS, int *N_CBPS, int *N_ES);

//�������ݱ���,ͬʱ��������
extern void GenInit(int *N_CBPS, int *N_SYM, int *ScrLength, int *valid_bits);//�������ݶ��ֽڳ��Ⱥ���Ч������
extern void GenDataAndScramble(unsigned char *data_scramble, int ScrLength, unsigned char *databits, int valid_bits); //���ݼ��ţ������ĸߵ�λ�ߵ�

//BCC����
extern void BCC_encoder(unsigned char *data_scramble, int ScrLength, int N_SYM, unsigned char **code_out, int CodeLength);
#ifdef OPTIMIZATION
extern void Creatnewchart(void);
extern void BCC_encoder_OPT(unsigned char *data_scramble, int ScrLength, int N_SYM, unsigned char **code_out, int CodeLength);
#endif
//���ƺ���
extern void modulate(unsigned char **code_out , int BCC_length, int N_SYM, complex32 **sym_mod, int *NumSampEffect );
extern void initial_streamwave_table(int N_SYM);
extern void init_mapping_table(void);
extern void modulate_mapping(unsigned char *BCC_output, complex32 **subcar_map_data);
//extern void modulate_mapping(unsigned char *BCC_output, unsigned char **stream_interweave_dataout, complex32 **subcar_map_data);

//���뵼Ƶ��Ƶ
extern void PilotAdd_SubcarMap(complex32 **sym_mod, int N_SYM, complex32 **subcar_map_data);
//CSD
extern void Data_CSD(complex32 **subcar_map_data, int N_SYM, complex32 **csd_data);
#ifdef AVX2
extern void __Data_CSD_aux(complex32 **subcar_map_data, int N_SYM, complex32 **csd_data,int NTXindex);//maybe use for multi pthread
extern void Matrix_Mult_AVX2_16(complex32 (*h)[16],complex32* x,complex32* dest);  //use for precoding
#endif
//IFFT
extern void csd_data_IDFT(complex32 *csd_data, complex32 *trans_data, int N_SYM);
extern void ifftShiftandIFFTData(complex32* dataAfterCSD,complex32* dataAfterIFFT);
extern void addCPforData(complex32* pAfterIFFT,complex32* pBeforeAddWin,int N_SYM, int symbol,int tx);
extern void add_window_for_he(complex32 **X_VHTLTF, complex32 *trans_data, complex32 window_buf[N_TX][3], complex32 **out);

#endif // PROCESS_DATA
