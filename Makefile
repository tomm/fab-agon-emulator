ifeq ($(OS),Windows_NT)
	OS_DETECTED := Windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OS_DETECTED := Linux
	endif
	ifeq ($(UNAME_S),Darwin)
		OS_DETECTED := Darwin
	endif
endif

all: check vdp cargo

COMPILER := $(filter g++ clang,$(shell $(CXX) --version))

check:
ifeq ($(OS_DETECTED),Windows)
	@if not exist ./src/vdp/userspace-vdp-gl/README.md ( echo Error: no source tree in ./src/vdp/userspace-vdp. && echo Maybe you forgot to run: git submodule update --init && exit /b 1 )
else
	@if [ ! -f ./src/vdp/userspace-vdp-gl/README.md ]; then echo "Error: no source tree in ./src/vdp/userspace-vdp."; echo "Maybe you forgot to run: git submodule update --init"; echo; exit 1; fi
endif

vdp:
ifeq ($(COMPILER),clang)
	# clang is strict about narrowing, where g++ is not
	EXTRA_FLAGS=-Wno-c++11-narrowing $(MAKE) -C src/vdp
else
	$(MAKE) -C src/vdp
endif

ifeq ($(OS_DETECTED),Windows)
	copy src\vdp\*.so firmware
else
	cp src/vdp/*.so firmware/
endif

cargo:
ifeq ($(OS_DETECTED),Windows)
	set FORCE=1 && cargo build -r
	copy target\release\fab-agon-emulator.exe .
else
	FORCE=1 cargo build -r
	cp ./target/release/fab-agon-emulator .
endif

vdp-clean:
	$(MAKE) -C src/vdp clean

cargo-clean:
	cargo clean

clean: vdp-clean cargo-clean

depends:
	$(MAKE) -C src/vdp depends
