CCOM = cc
# CCOM = /usr/local64/llvm-morello/bin/clang
# CFLAGS = -O0 -lpthread -lm -Iruntime
CFLAGS = -g -O2 -Wall -lpthread -lm
INCLUDES = -I../runtime
ASM = ../runtime/xbMrtime_api_asm.s
MY_CC = $(CCOM) $(CFLAGS) $(INCLUDES) $(ASM)
#DIR=~/cheri/output/rootfs-morello-purecap/mert_files/xbrtime-simple

all: matMul gather gups SHMEMRandomAccess SHMEMRandomAccess2 broadcast reduction

matMul:
	$(MY_CC) -o matmul.exe xbrtime_matmul.c

gather:
	$(MY_CC) -o gather.exe xbrtime_gather.c

gups:
	$(MY_CC) -o gups.exe xbrtime_gups.c

SHMEMRandomAccess:
	$(MY_CC) -o shmemRandomAccess.exe SHMEMRandomAccess.c

SHMEMRandomAccess2:
	$(MY_CC) -o shmemRandomAccess_v2.exe SHMEMRandomAccess_v2.c

broadcast:
	$(MY_CC) -o broadcast8.exe xbrtime_broadcast8.c
reduction:
	$(MY_CC) -o reduction8.exe xbrtime_reduction8.c

test:
	./matmul.exe
	./gather.exe
	./gups.exe
	./SHMEMRandomAccess.exe
	./broadcast8.exe
	./reduction8.exe

clean:
	rm -f ./*.o ./*.exe
