all: check vdp cargo

COMPILER := $(filter g++ clang,$(shell $(CXX) --version))
UNAME_S := $(shell uname)

check:
	@if [ ! -f ./src/vdp/userspace-vdp-gl/README.md ]; then echo "Error: no source tree in ./src/vdp/userspace-vdp."; echo "Maybe you forgot to run: git submodule update --init"; echo; exit 1; fi


vdp:
ifeq ($(UNAME_S),Darwin)
	EXTRA_FLAGS="-Wno-c++11-narrowing -arch x86_64" SUFFIX=.x86_64 $(MAKE) -C src/vdp
	EXTRA_FLAGS="-Wno-c++11-narrowing -arch arm64" SUFFIX=.arm64 $(MAKE) -C src/vdp
	$(MAKE) -C src/vdp lipo
	find src/vdp -type f \( -name "*.so" -a ! -name "*.x86_64.so" -a ! -name "*.arm64.so" \) -exec cp {} firmware/ \;
else
	$(MAKE) -C src/vdp
	cp src/vdp/*.so firmware/
endif

cargo:
ifeq ($(OS),Windows_NT)
	set FORCE=1 && cargo build -r
	cp ./target/release/fab-agon-emulator.exe .
else ifeq ($(UNAME_S),Darwin)
	FORCE=1 cargo build -r --target=x86_64-apple-darwin
	FORCE=1 cargo build -r --target=aarch64-apple-darwin
	lipo -create -output ./fab-agon-emulator ./target/x86_64-apple-darwin/release/fab-agon-emulator ./target/aarch64-apple-darwin/release/fab-agon-emulator
else
	FORCE=1 cargo build -r
	cp ./target/release/fab-agon-emulator .
endif

vdp-clean:
	rm -f firmware/*.so
ifeq ($(UNAME_S),Darwin)
	EXTRA_FLAGS="-Wno-c++11-narrowing -arch x86_64" SUFFIX=.x86_64 $(MAKE) -C src/vdp clean
	EXTRA_FLAGS="-Wno-c++11-narrowing -arch arm64" SUFFIX=.arm64 $(MAKE) -C src/vdp clean
else
	$(MAKE) -C src/vdp clean
endif

cargo-clean:
	rm -f fab-agon-emulator
	cargo clean

clean: vdp-clean cargo-clean

depends:
	$(MAKE) -C src/vdp depends

install:
ifneq ($(shell ./fab-agon-emulator --prefix),)
	install -D -t $(shell ./fab-agon-emulator --prefix)/share/fab-agon-emulator/ firmware/vdp_*.so
	install -D -t $(shell ./fab-agon-emulator --prefix)/share/fab-agon-emulator/ firmware/mos_*.bin
	install -D -t $(shell ./fab-agon-emulator --prefix)/share/fab-agon-emulator/ firmware/mos_*.map
	install -D -t $(shell ./fab-agon-emulator --prefix)/bin/ fab-agon-emulator
else
	@echo "make install requires an install PREFIX (eg PREFIX=/usr/local make)"
endif
