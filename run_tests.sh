#!/bin/sh
export LD_LIBRARY_PATH=`pwd`

echo "test_trace_exec.py"
python test_trace_exec.py $@

echo "test_tracecollector.py"
python test_tracecollector.py $@
