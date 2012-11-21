
CFLAGS = "-Wall"

trace-exec: trace-exec.c
	$(CC) $(CFLAGS) trace-exec.c -o trace-exec

libtrace-exec.so: trace-exec.c
	$(CC) -fpic -c trace-exec.c -o testlib.o
	$(CC) -shared testlib.o -o libtrace-exec.so
	rm testlib.o

.PHONY: test
test: libtrace-exec.so
	python test_trace_exec.py

.PHONY: clean
clean:
	rm libtrace-exec.so \
           trace-exec \
           trace-exec.o
