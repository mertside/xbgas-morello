CCOM = cc
# CCOM = /usr/local64/llvm-morello/bin/clang
# CFLAGS = -O0 -lpthread -lm -Iruntime
CFLAGS = -g -O2 -Wall -lpthread -lm
INCLUDES = -I../../runtime
ASM = ../../runtime/xbMrtime_api_asm.s
MY_CC = $(CCOM) $(CFLAGS) $(INCLUDES) $(ASM)

all: ttuC1B ttuC2B ttuC3B ttuC11B ttuC12B ttuC13B

ttuC1B:
	$(MY_CC) -o ttu_c1b_control_flow.exe ttu_c1b_control_flow.c
ttuC2B:
	$(MY_CC) -o ttu_c2b_rop.exe ttu_c2b_rop.c -fasm -fno-stack-protector
ttuC3B:
	$(MY_CC) -o ttu_c3b_use_after_realloc.exe ttu_c3b_use_after_realloc.c
ttuC11B:
	$(MY_CC) -o ttu_c11b_memory_leak.exe ttu_c11b_memory_leak.c
ttuC12B:
	$(MY_CC) -o ttu_c12b_race_condition.exe ttu_c12b_race_condition.c
ttuC13B:
	$(MY_CC) -o ttu_c13b_use_of_freed_memory.exe ttu_c13b_use_of_freed_memory.c
ttuT:
	$(MY_CC) -o ttu_t_hm_parent_and_child_chunk.exe ttu_t_hm_parent_and_child_chunk.c

clean:
	rm -f ./*.o ./*.exe ./*.core
