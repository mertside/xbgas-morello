#!/usr/bin/env bash
#
# Simple wrapper for running a benchmark under OpenSHMEM or xBGAS,
# collecting wall-clock time and cycle counter.  Assumes CheriBSD.
#
# Usage:
#    ./osh-run.sh ./shmem_vector_add_osh   8  10
#    ./osh-run.sh ./shmem_vector_add_xbg   8  10
#            ^exe ^num_PEs               ^iterations
#
###############################################################################

set -euo pipefail

BIN=$1          # benchmark executable
NP=$2           # number of processing elements
ITERS=${3:-1}   # outer repetitions (default 1)

export SHMEM_SYMMETRIC_HEAP_SIZE=512M   # common heap for both runtimes
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-/usr/local/lib}

run_once() {
	/usr/bin/time -f "elapsed_sec=%e" \
		oshrun -np "${NP}" --map-by ppr:${NP}:node "${BIN}" --iters "${ITERS}"
}

echo "## $(date)  BIN=${BIN}  NP=${NP}  ITERS=${ITERS}"
echo "## hostname: $(hostname)   cpu: $(sysctl -n machdep.cpu.brand_string)"
echo "## --------- Run 5× (first warm-up discarded) ----------"

for i in 1 2 3 4 5; do
	echo -n "run_$i "
	run_once
done