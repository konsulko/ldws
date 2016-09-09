# Copyright 2016 Konsulko Group
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

#!/bin/bash

CFGS=("examples/road-single.conf" "examples/road-dual.conf")
OPTS=( "" "--enable-opencl" "--enable-cuda" )

if [ -z "$1" ]
then
      echo "Usage: $0 <output directory>"
      exit 1
fi
ODIR=$1
mkdir -p $ODIR

for cfg in "${CFGS[@]}"
do
	for opt in "${OPTS[@]}"
	do
		filespec=$(basename "$cfg")
		filename="${filespec%.*}"
		if [ "$opt" = "--enable-cuda" ];
		then
			mode="cuda"
		elif [ "$opt" = "--enable-opencl" ];
		then
			mode="opencl"
		else
			mode="cpu"
		fi

		# Get a perf stat summary
		perf stat ./ldws --config-file $cfg $opt > $ODIR/perf-stat-$filename-$mode.out 2>&1

		# Record call graph
		perf record -F99 --call-graph dwarf -o $ODIR/perf-$filename-$mode.dat ./ldws --config-file $cfg $opt

		# Generate text-based hierarchical call graph
		perf report -i $ODIR/perf-$filename-$mode.dat  --stdio -n --hierarchy --percent-limit 2 > $ODIR/perf-report-$filename-$mode.out

		# Generate graphical callgraph
		perf script -i $ODIR/perf-$filename-$mode.dat | c++filt | gprof2dot.py -n 3 -z main -s -f perf | dot -Tpng -o $ODIR/callgraph-$filename-$mode.png

		# Generate flamegraph
		perf script -i $ODIR/perf-$filename-$mode.dat | stackcollapse-perf.pl > $ODIR/perf-$filename-$mode-folded.dat
		grep ldws $ODIR/perf-$filename-$mode-folded.dat | flamegraph.pl --fontsize 8 > $ODIR/flamegraph-$filename-$mode.svg

		# Generate nvprof summary and nvvp profiler data for CUDA runs (CPU APIS + GPU kernel activity)
		if [ "$mode" = "cuda" ];
		then
			nvprof -s -o $ODIR/nvprof-$filename-$mode.dat ./ldws --config-file $cfg $opt > $ODIR/nvprof-$filename-$mode.out 2>&1
		fi

		# Generate nvvp profiler data for OpenCL runs (GPU kernel activity only due to legacy command line profiler limitations)
		if [ "$mode" = "opencl" ];
		then
			COMPUTE_PROFILE=1 COMPUTE_PROFILE_CONFIG=tools/nvvp.cfg COMPUTE_PROFILE_LOG=$ODIR/legacyprof-$filename-$mode.log ./ldws --config-file $cfg $opt > $ODIR/legacyprof-$filename-$mode.out 2>&1
			sed -f tools/opencl2cuda.sed's/OPENCL_/CUDA_/g' $ODIR/legacyprof-$filename-$mode.log > $ODIR/nvvp-cmdlineprof-$filename-$mode.log
		fi
	done
done
