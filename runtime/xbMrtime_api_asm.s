#
# _XBRTIME_API_ASM_S_
#

  .file "xbrtime_api_asm.s"
  .text
  .align 1
  #---------------------------------------------------
  # xBGAS ASM API functions are formatted as follows:
  # __xbrtime_{get,put}_OPERAND_{seq,agg}
  #   where,
  #   - {get,put} correponds to receive and send
  #   - OPERAND is one of:
  #     - u1 = unsigned one byte
  #     - u2 = unsigned two byte
  #     - u4 = unsigned four byte
  #     - u8 = unsigned eight byte
  #     - s1 = signed one byte
  #     - s2 = signed two byte
  #     - s4 = signed four byte
  #     - s8 = signed eight byte
  #   - {seq,agg} corresponds to sequential
  #             and aggregated transfers
  #---------------------------------------------------
  # Calling Convention 
  # ARM Morello = RISCV = Explanation
  #        - X0 = a0    = base src address
  #        - X1 = a1    = base dest address
  #        -    = a2    = remote pe
  #        - X2 = a3    = nelems
  #        - X3 = a4    = stride (in bytes)
  #---------------------------------------------------

  #--------------------------------------------------------------------- U1 SEQ

  .global __xbrtime_get_u1_seq
  .type __xbrtime_get_u1_seq, @function
__xbrtime_get_u1_seq:
  MOV X12, XZR
.get_u1_seq:
  LDRB W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRB W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_u1_seq
  RET
  .size __xbrtime_get_u1_seq, .-__xbrtime_get_u1_seq

  #---------------------------------------------------

  .global __xbrtime_put_u1_seq
  .type __xbrtime_put_u1_seq, @function
__xbrtime_put_u1_seq:
  MOV X12, XZR
.put_u1_seq:
  LDRB W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRB W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_u1_seq
  RET
  .size __xbrtime_put_u1_seq, .-__xbrtime_put_u1_seq

  #--------------------------------------------------------------------- S1 SEQ

  .global __xbrtime_get_s1_seq
  .type __xbrtime_get_s1_seq, @function
__xbrtime_get_s1_seq:
  MOV X12, XZR
.get_s1_seq:
  LDRB W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRB W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_s1_seq
  RET
  .size __xbrtime_get_s1_seq, .-__xbrtime_get_s1_seq

  #---------------------------------------------------

  .global __xbrtime_put_s1_seq
  .type __xbrtime_put_s1_seq, @function
__xbrtime_put_s1_seq:
  MOV X12, XZR
.put_s1_seq:
  LDRB W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRB W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_s1_seq
  RET
  .size __xbrtime_put_s1_seq, .-__xbrtime_put_s1_seq

  #--------------------------------------------------------------------- U2 SEQ

  .global __xbrtime_get_u2_seq
  .type __xbrtime_get_u2_seq, @function
__xbrtime_get_u2_seq:
  MOV X12, XZR
.get_u2_seq:
  LDRH W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRH W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_u2_seq
  RET
  .size __xbrtime_get_u2_seq, .-__xbrtime_get_u2_seq

  #---------------------------------------------------

  .global __xbrtime_put_u2_seq
  .type __xbrtime_put_u2_seq, @function
__xbrtime_put_u2_seq:
  MOV X12, XZR
.put_u2_seq:
  LDRH W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRH W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_u2_seq
  RET
  .size __xbrtime_put_u2_seq, .-__xbrtime_put_u2_seq

  #--------------------------------------------------------------------- S2 SEQ

  .global __xbrtime_get_s2_seq
  .type __xbrtime_get_s2_seq, @function
__xbrtime_get_s2_seq:
  MOV X12, XZR
.get_s2_seq:
  LDRH W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRH W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_s2_seq
  RET
  .size __xbrtime_get_s2_seq, .-__xbrtime_get_s2_seq

  #---------------------------------------------------

  .global __xbrtime_put_s2_seq
  .type __xbrtime_put_s2_seq, @function
__xbrtime_put_s2_seq:
  MOV X12, XZR
.put_s2_seq:
  LDRH W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STRH W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_s2_seq
  RET
  .size __xbrtime_put_s2_seq, .-__xbrtime_put_s2_seq

  #--------------------------------------------------------------------- U4 SEQ

  .global __xbrtime_get_u4_seq
  .type __xbrtime_get_u4_seq, @function
__xbrtime_get_u4_seq:
  MOV X12, XZR
.get_u4_seq:
  LDR W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_u4_seq
  RET
  .size __xbrtime_get_u4_seq, .-__xbrtime_get_u4_seq

  #---------------------------------------------------

  .global __xbrtime_put_u4_seq
  .type __xbrtime_put_u4_seq, @function
__xbrtime_put_u4_seq:
  MOV X12, XZR
.put_u4_seq:
  LDR W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_u4_seq
  RET
  .size __xbrtime_put_u4_seq, .-__xbrtime_put_u4_seq

  #--------------------------------------------------------------------- S4 SEQ

  .global __xbrtime_get_s4_seq
  .type __xbrtime_get_s4_seq, @function
__xbrtime_get_s4_seq:
  MOV X12, XZR
.get_s4_seq:
  LDR W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_s4_seq
  RET
  .size __xbrtime_get_s4_seq, .-__xbrtime_get_s4_seq

  #---------------------------------------------------

  .global __xbrtime_put_s4_seq
  .type __xbrtime_put_s4_seq, @function
__xbrtime_put_s4_seq:
  MOV X12, XZR
.put_s4_seq:
  LDR W10, [X0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR W10, [X1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_s4_seq
  RET
  .size __xbrtime_put_s4_seq, .-__xbrtime_put_s4_seq

  #--------------------------------------------------------------------- U8 SEQ

  .global __xbrtime_get_u8_seq
  .type __xbrtime_get_u8_seq, @function
__xbrtime_get_u8_seq:
  MOV X12, XZR
.get_u8_seq:
  LDR X10, [X0] 
  #[C0]
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR X10, [X1] 
  #[C1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_u8_seq
  RET
  .size __xbrtime_get_u8_seq, .-__xbrtime_get_u8_seq

  #---------------------------------------------------

  .global __xbrtime_put_u8_seq
  .type __xbrtime_put_u8_seq, @function
__xbrtime_put_u8_seq:
  MOV X12, XZR
.put_u8_seq:
  LDR X10, [X0] 
  #[C0] 
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR X10, [X1] 
  #[C1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_u8_seq
  RET
  .size __xbrtime_put_u8_seq, .-__xbrtime_put_u8_seq

  #--------------------------------------------------------------------- S8 SEQ

  .global __xbrtime_get_s8_seq
  .type __xbrtime_get_s8_seq, @function
__xbrtime_get_s8_seq:
  MOV X12, XZR
.get_s8_seq:
  LDR X10, [X0] 
  #[C0] 
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR X10, [X1] 
  #[C1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .get_s8_seq
  RET
  .size __xbrtime_get_s8_seq, .-__xbrtime_get_s8_seq

  #---------------------------------------------------

  .global __xbrtime_put_s8_seq
  .type __xbrtime_put_s8_seq, @function
__xbrtime_put_s8_seq:
  MOV X12, XZR
.put_s8_seq:
  LDR X10, [X0] 
  #[C0] 
  ADD X0, X0, X3
  ADD X12, X12, #1
  STR X10, [X1] 
  #[C1]
  ADD X1, X1, X3
  CMP X12, X2
  BNE .put_s8_seq
  RET
  .size __xbrtime_put_s8_seq, .-__xbrtime_put_s8_seq

  #---------------------------------------------------------------------- FENCE
  
  .global __xbrtime_asm_fence
  .type __xbrtime_asm_fence, @function
__xbrtime_asm_fence:
  DSB SY
  ISB
  RET
  .size __xbrtime_asm_fence, .-__xbrtime_asm_fence

  #---------------------------------------------------

  .global __xbrtime_asm_quiet_fence
  .type __xbrtime_asm_quiet_fence, @function
__xbrtime_asm_quiet_fence:
  DMB SY
  RET
  .size __xbrtime_asm_quiet_fence, .-__xbrtime_asm_quiet_fence
  
  #-- EOF
