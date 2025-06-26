CCOM = cc
CFLAGS = -g -O2 -Wall 
MY_CC = $(CCOM) $(CFLAGS)

all: asuS1 asuS2 asuS3 asuS4 asuS5 asuT1 asuT2 asuT3 asuT4 asuT5 asuT6 asuT7 asuR1 asuR2 asuR3 asuR4 asuR5

temporal: asuT1 asuT2 asuT3 asuT4 asuT5 asuT6 asuT7

spatial: asuS1 asuS2 asuS3 asuS4 asuS5

real: asuR1 asuR2 asuR3 asuR4 asuR5

heap: asuT2 asuT3 asuT4

asuS1:
	$(MY_CC) -o asu_s1_free_not_at_start.exe asu_s1_free_not_at_start.c

asuS2:
	$(MY_CC) -o asu_s2_free_not_on_heap.exe asu_s2_free_not_on_heap.c

asuS3:
	$(MY_CC) -o asu_s3_null_ptr_dereference.exe asu_s3_null_ptr_dereference.c

asuS4:
	$(MY_CC) -o asu_s4_oob_read.exe asu_s4_oob_read.c

asuS5:
	$(MY_CC) -o asu_s5_oob_write.exe asu_s5_oob_write.c

asuT1:
	$(MY_CC) -o asu_t1_double_free.exe asu_t1_double_free.c

asuT2:
	$(MY_CC) -o asu_t2_hm_fake_chunk_malloc.exe asu_t2_hm_fake_chunk_malloc.c

asuT3:
	$(MY_CC) -o asu_t3_hm_house_of_spirit.exe asu_t3_hm_house_of_spirit.c

asuT4:
	$(MY_CC) -o asu_t4_hm_parent_and_child_chunk.exe asu_t4_hm_parent_and_child_chunk.c

asuT5:
	$(MY_CC) -o asu_t5_use_after_free.exe asu_t5_use_after_free.c

asuT6:
	$(MY_CC) -o asu_t6_uaf_function_pointer.exe asu_t6_uaf_function_pointer.c

asuT7:
	$(MY_CC) -o asu_t7_uaf_memcpy.exe asu_t7_uaf_memcpy.c

asuR1:
	$(MY_CC) -o asu_r1_heartbleed.exe asu_r1_heartbleed.c

asuR2:
	$(MY_CC) -o asu_r2_dop.exe asu_r2_dop.c

asuR3:
	$(MY_CC) -o asu_r3_uaf_to_code_reuse.exe asu_r3_uaf_to_code_reuse.c

asuR4:
	$(MY_CC) -o asu_r4_ip_large_size.exe asu_r4_ip_large_size.c

asuR5:
	$(MY_CC) -o asu_r5_df_switch_statements.exe asu_r5_df_switch_statements.c

runS:
	./asu_s1_free_not_at_start.exe
	./asu_s2_free_not_on_heap.exe
	./asu_s3_null_ptr_dereference.exe
	./asu_s4_oob_read.exe
	./asu_s5_oob_write.exe

runT:
	./asu_t1_double_free.exe
	./asu_t2_hm_fake_chunk_malloc.exe
	./asu_t3_hm_house_of_spirit.exe
	./asu_t4_hm_parent_and_child_chunk.exe
	./asu_t5_use_after_free.exe
	./asu_t6_uaf_function_pointer.exe
	./asu_t7_uaf_memcpy.exe

runR:
	./asu_r1_heartbleed.exe
	./asu_r2_dop.exe
	./asu_r3_uaf_to_code_reuse.exe
	./asu_r4_ip_large_size.exe
	./asu_r5_df_switch_statements.exe

runA:
	./asu_s1_free_not_at_start.exe
	./asu_s2_free_not_on_heap.exe
	./asu_s3_null_ptr_dereference.exe
	./asu_s4_oob_read.exe
	./asu_s5_oob_write.exe
	./asu_t1_double_free.exe
	./asu_t2_hm_fake_chunk_malloc.exe
	./asu_t3_hm_house_of_spirit.exe
	./asu_t4_hm_parent_and_child_chunk.exe
	./asu_t5_use_after_free.exe
	./asu_t6_uaf_function_pointer.exe
	./asu_t7_uaf_memcpy.exe
	./asu_r1_heartbleed.exe
	./asu_r2_dop.exe
	./asu_r3_uaf_to_code_reuse.exe
	./asu_r4_ip_large_size.exe
	./asu_r5_df_switch_statements.exe

clean:
	rm -f ./*.o ./*.exe ./*.core
