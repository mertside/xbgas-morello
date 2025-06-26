#!/bin/sh
echo "============================================================"
echo "Running ASU tests" 

echo " "
echo "DATE: " 
date

echo " "
echo "UNAME: " 
uname -a

echo " "
echo "SYSCTL: " 
sysctl -a | egrep -i 'hw.machine|hw.model|hw.ncpu|hw.usermem'

echo " "
echo "CC: "
cc --version

export NUM_OF_THREADS=4

# ----------------------------------------------------------------------------
echo "============================================================"
echo " "

echo "------------------------------"
echo "ASU_S1: Free not at start"
asuS1=`./asu_s1_free_not_at_start.exe`;
echo "$asuS1"
echo " "

echo "------------------------------"
echo "ASU_S2: Free not on heap"
asuS2=`./asu_s2_free_not_on_heap.exe`;
echo "$asuS2"
echo " "

echo "------------------------------"
echo "ASU_S3: Null pointer dereference"
asuS3=`./asu_s3_null_ptr_dereference.exe`;
echo "$asuS3"
echo " "

echo "------------------------------"
echo "ASU_S4: Out of bounds read"
asuS4=`./asu_s4_oob_read.exe`;
echo "$asuS4"
echo " "

echo "------------------------------"
echo "ASU_S5: Out of bounds write"
asuS5=`./asu_s5_oob_write.exe`;
echo "$asuS5"
echo " "

# ----------------------------------------------------------------------------
echo "============================================================"
echo " "

echo "------------------------------"
echo "ASU_T1: Double Free"
asuT1=`./asu_t1_double_free.exe`;
echo "$asuT1"
echo " "

echo "------------------------------"
echo "ASU_T2: HM - Fake Chunk Malloc"
asuT2=`./asu_t2_hm_fake_chunk_malloc.exe`;
echo "$asuT2"
echo " "

echo "------------------------------"
echo "ASU_T3: HM - House Of Spirit"
asuT3=`./asu_t3_hm_house_of_spirit.exe`;
echo "$asuT3"
echo " "

echo "------------------------------"
echo "ASU_T4: HM - Parent and Child Chunk"
asuT4=`./asu_t4_hm_parent_and_child_chunk.exe`;
echo "$asuT4"
echo " "

echo "------------------------------"
echo "ASU_T5: Use After Free"
asuT5=`./asu_t5_use_after_free.exe`;
echo "$asuT5"
echo " "

echo "------------------------------"
echo "ASU_T6: UAF Function Pointer"
asuT6=`./asu_t6_uaf_function_pointer.exe`;
echo "$asuT6"
echo " "

echo "------------------------------"
echo "ASU_T7: UAF Memcpy"
asuT7=`./asu_t7_uaf_memcpy.exe`;
echo "$asuT7"
echo " "

# ----------------------------------------------------------------------------
echo "============================================================"
echo " "

echo "------------------------------"
echo "ASU_R1: Heartbleed"
asuR1=`./asu_r1_heartbleed.exe`;
echo "$asuR1"
echo " "

echo "------------------------------"
echo "ASU_R2: DOP"
asuR2=`./asu_r2_dop.exe`;
echo "$asuR2"
echo " "

echo "------------------------------"
echo "ASU_R3: UAF to code reuse"
asuR3=`./asu_r3_uaf_to_code_reuse.exe`;
echo "$asuR3"
echo " "

echo "------------------------------"
echo "ASU_R4: Illegal pointer dereference"
asuR4=`./asu_r4_ip_large_size.exe`;
echo "$asuR4"
echo " "

echo "------------------------------"
echo "ASU_R5: Double Free on Switch statements"
asuR5=`./asu_r5_df_switch_statements.exe`;
echo "$asuR5"
echo " "

# ----------------------------------------------------------------------------
echo "============================================================"
echo " "