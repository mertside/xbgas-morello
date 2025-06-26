# Compiler & flags ------------------------------------------------------------
CC          := cc
# CFLAGS      := -O3 -mcpu=morello -pipe -fno-strict-aliasing
CFLAGS 			:= -g -O2 -Wall -lpthread -lm
LDFLAGS     :=
LIB_SHMEM   := -loshmem    # OpenSHMEM (linked via OpenMPI)
# LIB_XBGAS   := -lxbrtime   # secure runtime   (libxbrtime.a / .so)
INCLUDES 		:= -I../../runtime
ASM 				:= ../../runtime/xbMrtime_api_asm.s
LIB_XBGAS 	:= $(INCLUDES) $(ASM)

# Purecap vs. non-purecap build knobs ----------------------------------------
# OpenSHMEM uses legacy 64-bit pointers; xBGAS build is purecap.
# The flags below keep both in 128-bit mode for apples-to-apples comparison.
# CFLAGS      += -cheri=128
# If you also want a legacy (non-cap) OpenSHMEM build, uncomment:
# LEGACY_FLAGS := -mno-purecap -mabi=aapcs        # 64-bit pointers, same ISA

# Source files ---------------------------------------------------------------
# Add new benchmarks here; pattern rule handles both variants automatically.
SRC         := shmem_ping_pong.c                 \
               shmem_sum_example.c               \
               shmem_vector_add.c                \
               stream_triad.c

# Derive target names:  foo.c -> foo_osh and foo_xbg
OSH_TGT      := $(addsuffix _osh,  $(basename $(SRC)))
XBG_TGT      := $(addsuffix _xbg,  $(basename $(SRC)))

# Default target -------------------------------------------------------------
all: $(OSH_TGT) $(XBG_TGT)

# Pattern rules --------------------------------------------------------------
# 1) OpenSHMEM variant  (legacy flags optional)
%_osh: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LIB_SHMEM) $(LDFLAGS) 

# 2) xBGAS-runtime variant (purecap mandatory)
%_xbg: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LIB_XBGAS) $(LDFLAGS)

# Clean ----------------------------------------------------------------------
clean:
	rm -f $(OSH_TGT) $(XBG_TGT)

# # Compiler and Flags
# CC = cc
# CFLAGS = -O2 -Wall
# LDFLAGS = -lm
# SHMEM_LIBS = -lshmem

# # Source and Object Files
# SRCS = shmem_ping_pong.c shmem_sum_example.c shmem_vector_add.c stream_triad.c
# BINS = $(SRCS:.c=)

# # Default target
# all: $(BINS)

# # Compile each binary
# %: %.c
# 	$(CC) $(CFLAGS) -o $@ $< $(SHMEM_LIBS) $(LDFLAGS)

# # Clean build
# clean:
# 	rm -f $(BINS)

# # Run one binary (example: make run_ping)
# run_%: %
# 	./$*

# .PHONY: all clean