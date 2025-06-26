CCOM = cc
# CCOM = /usr/local64/llvm-morello/bin/clang
# CFLAGS = -O0 -lpthread -lm -Iruntime
CFLAGS = -g -O2 -Wall -lpthread -lm
INCLUDES = -I../../runtime
ASM = ../../runtime/xbMrtime_api_asm.s
MY_CC = $(CCOM) $(CFLAGS) $(INCLUDES) $(ASM)

all: ttuS1 ttuS2 ttuS3 ttuS4 ttuS5 ttuT1 ttuT2 ttuT3 ttuT4 ttuT5 ttuT6 ttuT7 ttuR1 ttuR2 ttuR3 ttuR4 ttuR5

temporal: ttuT1 ttuT2 ttuT3 ttuT4 ttuT5 ttuT6 ttuT7

spatial: ttuS1 ttuS2 ttuS3 ttuS4 ttuS5

real: ttuR1 ttuR2 ttuR3 ttuR4 ttuR5

heap: ttuT2 ttuT3 ttuT4

ttuS1:
	$(MY_CC) -o ttu_s1_free_not_at_start.exe ttu_s1_free_not_at_start.c
ttuS2:
	$(MY_CC) -o ttu_s2_free_not_on_heap.exe ttu_s2_free_not_on_heap.c
ttuS3:
	$(MY_CC) -o ttu_s3_null_ptr_dereference.exe ttu_s3_null_ptr_dereference.c
ttuS4:
	$(MY_CC) -o ttu_s4_oob_read.exe ttu_s4_oob_read.c
ttuS5:
	$(MY_CC) -o ttu_s5_oob_write.exe ttu_s5_oob_write.c

ttuT1:
	$(MY_CC) -o ttu_t1_double_free.exe ttu_t1_double_free.c
ttuT2:
	$(MY_CC) -o ttu_t2_hm_fake_chunk_malloc.exe ttu_t2_hm_fake_chunk_malloc.c
ttuT3:
	$(MY_CC) -o ttu_t3_hm_house_of_spirit.exe ttu_t3_hm_house_of_spirit.c
ttuT4:
	$(MY_CC) -o ttu_t4_hm_p_and_c_chunk.exe ttu_t4_hm_p_and_c_chunk.c
ttuT5:
	$(MY_CC) -o ttu_t5_use_after_free.exe ttu_t5_use_after_free.c
ttuT6:
	$(MY_CC) -o ttu_t6_uaf_function_pointer.exe ttu_t6_uaf_function_pointer.c
ttuT7:
	$(MY_CC) -o ttu_t7_uaf_memcpy.exe ttu_t7_uaf_memcpy.c

ttuR1:
	$(MY_CC) -o ttu_r1_HeartBleed.exe ttu_r1_HeartBleed.c
ttuR2:
	$(MY_CC) -o ttu_r2_dop.exe ttu_r2_dop.c
ttuR3:
	$(MY_CC) -o ttu_r3_uaf_to_code_reuse.exe ttu_r3_uaf_to_code_reuse.c
ttuR4:
	$(MY_CC) -o ttu_r4_illegal_ptr_deref.exe ttu_r4_illegal_ptr_deref.c
ttuR5:
	$(MY_CC) -o ttu_r5_df_switch.exe ttu_r5_df_switch.c

ttuC1B:
	$(MY_CC) -o ttu_c1_baseline_use_of_freed_memory.exe ttu_c1_baseline_use_of_freed_memory.c
ttuC2B:
	$(MY_CC) -o ttu_c2_baseline_race_condition.exe ttu_c2_baseline_race_condition.c
ttuC3B:
	$(MY_CC) -o ttu_c3_baseline_memory_leak.exe ttu_c3_baseline_memory_leak.c
ttuC4B:
	$(MY_CC) -o ttu_c4_control_flow.exe ttu_c4_control_flow.c
ttuC5B:
	$(MY_CC) -o ttu_c5_rop.exe ttu_c5_rop.c

ttuS4B:
	$(MY_CC) -o ttu_s4_baseline_oob_read.exe ttu_s4_baseline_oob_read.c
ttuS5B:
	$(MY_CC) -o ttu_s5_baseline_oob_write.exe ttu_s5_baseline_oob_write.c

runS:
	./ttu_s1_free_not_at_start.exe
	./ttu_s2_free_not_on_heap.exe
	./ttu_s3_null_ptr_dereference.exe
	./ttu_s4_oob_read.exe
	./ttu_s5_oob_write.exe

runT:
	./ttu_t1_double_free.exe
	./ttu_t2_hm_fake_chunk_malloc.exe
	./ttu_t3_hm_house_of_spirit.exe
	./ttu_t4_hm_p_and_c_chunk.exe
	./ttu_t5_use_after_free.exe
	./ttu_t6_uaf_function_pointer.exe
	./ttu_t7_uaf_memcpy.exe

runR:
	./ttu_r1_HeartBleed.exe
	./ttu_r2_dop.exe
	./ttu_r3_uaf_to_code_reuse.exe
	./ttu_r4_illegal_ptr_deref.exe
	./ttu_r5_df_switch.exe

runA:
	./ttu_s1_free_not_at_start.exe
	./ttu_s2_free_not_on_heap.exe
	./ttu_s3_null_ptr_dereference.exe
	./ttu_s4_oob_read.exe
	./ttu_s5_oob_write.exe
	./ttu_t1_double_free.exe
	./ttu_t2_hm_fake_chunk_malloc.exe
	./ttu_t3_hm_house_of_spirit.exe
	./ttu_t4_hm_p_and_c_chunk.exe
	./ttu_t5_use_after_free.exe
	./ttu_t6_uaf_function_pointer.exe
	./ttu_t7_uaf_memcpy.exe
	./ttu_r1_HeartBleed.exe
	./ttu_r2_dop.exe
	./ttu_r3_uaf_to_code_reuse.exe
	./ttu_r4_illegal_ptr_deref.exe
	./ttu_r5_df_switch.exe

clean:
	rm -f ./*.o ./*.exe ./*.core
