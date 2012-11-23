#!/bin/sh
export LD_LIBRARY_PATH=`pwd`
python test_trace_exec.py $@
