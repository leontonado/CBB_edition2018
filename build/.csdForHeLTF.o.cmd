cmd_csdForHeLTF.o = gcc -Wp,-MD,./.csdForHeLTF.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/home/zgp/my_project/CBB-base-edition/build/include -I/home/zgp/my_project/CBB-base-edition/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include -include /home/zgp/my_project/CBB-base-edition/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include/rte_config.h -O1 -I/home/zgp/my_project/CBB-base-edition/BCCencode -I/home/zgp/my_project/CBB-base-edition/IFFT -I/home/zgp/my_project/CBB-base-edition/intrinsics_interface -I/home/zgp/my_project/CBB-base-edition/Process -I/home/zgp/my_project/CBB-base-edition/process_data -I/home/zgp/my_project/CBB-base-edition/process_data/process_datafunction -I/home/zgp/my_project/CBB-base-edition/typeDef -I/home/zgp/my_project/CBB-base-edition/VarINIT -D OPTIMIZATION -D AVX2    -o csdForHeLTF.o -c /home/zgp/my_project/CBB-base-edition/Process/csdForHeLTF.c 
