
CFLAGS = -Wall

SRCS = trace-exec.c transport.c 
OBJS = $(SRCS:.c=.o)

trace-exec: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o trace-exec

libtrace-exec.so: $(SRCS)
	$(CC) -fpic -c trace-exec.c -o libtrace-exec.o
	$(CC) -fpic -c transport.c -o libtrace-transport.o
	$(CC) -shared libtrace-exec.o libtrace-transport.o -o libtrace-exec.so
	rm libtrace*.o

.PHONY: test
test: libtrace-exec.so
	LD_LIBRARY_PATH=`pwd` python test_trace_exec.py

.PHONY: clean
clean:
	rm -f *.so *.o trace-exec
