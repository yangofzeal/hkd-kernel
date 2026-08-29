CC ?= cc
CFLAGS ?= -O3 -DNDEBUG -Wall -Wextra -Iinclude
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LIB = libhkd_kernel.dylib
LDFLAGS_SHARED = -dynamiclib
else
LIB = libhkd_kernel.so
LDFLAGS_SHARED = -shared -fPIC
endif

BENCHES = bench_native bench_dense bench_app
all: $(LIB) $(BENCHES)

$(LIB): src/hkd_kernel.c include/hkd_kernel.h
	$(CC) $(CFLAGS) -fPIC $(LDFLAGS_SHARED) src/hkd_kernel.c -o $@

bench_native: benchmark/bench_native.c src/hkd_kernel.c include/hkd_kernel.h
	$(CC) $(CFLAGS) benchmark/bench_native.c src/hkd_kernel.c -o $@
bench_dense: benchmark/bench_dense.c src/hkd_kernel.c include/hkd_kernel.h
	$(CC) $(CFLAGS) benchmark/bench_dense.c src/hkd_kernel.c -o $@
bench_app: benchmark/bench_app.c src/hkd_kernel.c include/hkd_kernel.h
	$(CC) $(CFLAGS) benchmark/bench_app.c src/hkd_kernel.c -o $@

clean:
	rm -f libhkd_kernel.so libhkd_kernel.dylib $(BENCHES)
