
CC=gcc
LD=ld
AR=ar
TAR=tar
BINEXT=
TOOLS=lxGcc64

ifndef PROJECT_VERSION
	# We just provide a primitive version string so we can see which build
	# we're using while debugging. For a release we will override the version
	# by providing it from cli args to 'make' as in:
	#     make clean package PROJECT_VERSION=1.2.3
	PROJECT_VERSION=$(shell date +0.0.0-%Y%m%d.%H%M%S)
endif

CFLAGS= --std=c99                                                             \
	-Wall -Wextra -Werror -fmax-errors=3                                      \
	-Wno-error=unused-function -Wno-error=unused-label                        \
	-Wno-error=unused-variable -Wno-error=unused-parameter                    \
	-Wno-error=unused-const-variable                                          \
	-Werror=implicit-fallthrough=1                                            \
	-Wno-error=unused-but-set-variable                                        \
	-Wno-unused-function -Wno-unused-parameter                                \
	-DPROJECT_VERSION=$(PROJECT_VERSION)

LDFLAGS= -Wl,--no-demangle,--fatal-warnings

INCDIRS= -Isrc/bulk_ln -Isrc/common

ifndef NDEBUG
	CFLAGS := $(CFLAGS) -ggdb -O0 -g3
else
	CFLAGS := $(CFLAGS) -ffunction-sections -fdata-sections -Os "-DNDEBUG=1"
	LDFLAGS := $(LDFLAGS) -Wl,--gc-sections,--as-needed
endif


default: link package

.PHONY: clean
clean:
	@echo "\n[INFO ] Clean"
	rm -rf build dist

.PHONY: link
link: build/bin/bulk-ln$(BINEXT)

build/obj/%.o: src/%.c
	@echo "\n[INFO ] Compile '$@'"
	@mkdir -p $(shell dirname build/obj/$*)
	$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

build/bin/bulk-ln$(BINEXT): \
		build/obj/bulk_ln/bulk_ln.o \
		build/obj/bulk_ln/bulk_ln_main.o
	@echo "\n[INFO ] Link '$@'"
	@mkdir -p $(shell dirname $@)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBSDIR)

.PHONY: package
package: link
	@echo "\n[INFO ] Package"
	@rm -rf build/dist-* dist
	@mkdir dist
	@echo
	@bash -c 'if [[ -n `git status --porcelain` ]]; then echo "[ERROR] Worktree not clean as it should be (see: git status)"; exit 1; fi'
	@# Create Executable bundle.
	@rm -rf build/dist-bin && mkdir -p build/dist-bin
	@cp -t build/dist-bin \
		README*
	@mkdir build/dist-bin/bin
	@cp -t build/dist-bin/bin \
		build/bin/*$(BINEXT)
	@(cd build/dist-bin && find . -type f -not -name MD5SUM -exec md5sum -b {} \;) > build/MD5SUM
	@mv build/MD5SUM build/dist-bin/.
	@(cd build/dist-bin && $(TAR) --owner=0 --group=0 -czf ../../dist/BulkLn-$(PROJECT_VERSION)-$(TOOLS).tgz *)
	@echo "\n[INFO ] DONE: Artifacts created and placed in 'dist'."
	@echo
	@echo See './dist/' for result.

