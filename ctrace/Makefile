CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 \
         -O2 -march=native \
	 -Wall -Werror -pedantic

CTRACE_OBJS = ctrace.o argv.o masq.o ccmd.o quote.o transport.o

DEPS = ctrace.h


all: ctrace libctrace.so

ctrace: $(CTRACE_OBJS)
	$(CC) $^ -o $@

libctrace.so: $(CTRACE_OBJS)
	$(CC) -shared $^ -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -fpic -c $< -o $@

.PHONY: clean
clean:
	rm ctrace libctrace.so *.o

.PHONY: analyze
analyze:
	$(MAKE) clean
	scan-build -V $(MAKE)
