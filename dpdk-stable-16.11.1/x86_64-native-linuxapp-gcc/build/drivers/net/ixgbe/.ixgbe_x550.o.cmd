cmd_ixgbe_x550.o = gcc -Wp,-MD,./.ixgbe_x550.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/home/zgp/my_project/CBB-base-edition/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include -include /home/zgp/my_project/CBB-base-edition/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include/rte_config.h -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-deprecated -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-unused-parameter -Wno-unused-value -Wno-strict-aliasing -Wno-format-extra-args   -o ixgbe_x550.o -c /home/zgp/my_project/CBB-base-edition/dpdk-stable-16.11.1/drivers/net/ixgbe/base/ixgbe_x550.c 
