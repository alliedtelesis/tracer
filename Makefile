
CFLAGS = -Wall

SRCS = trace-exec.c transport.c 
OBJS = $(SRCS:.c=.o)

trace-exec: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o trace-exec

libtrace-exec.so: $(SRCS)
	$(CC) -fpic -c trace-exec.c -o testlib.o
	$(CC) -shared testlib.o -o libtrace-exec.so
	rm testlib.o

.PHONY: test
test: libtrace-exec.so
	python test_trace_exec.py

.PHONY: clean
clean:
	rm *.so *.o trace-exec
