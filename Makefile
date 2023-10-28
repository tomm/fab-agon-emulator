all: check vdp cargo

check:
ifeq ($(OS),Windows_NT)
	@if not exist ./src/vdp/userspace-vdp-gl/README.md ( echo Error: no source tree in ./src/vdp/userspace-vdp. && echo Maybe you forgot to run: git submodule update --init && exit /b 1 )
else
	@if [ ! -f ./src/vdp/userspace-vdp-gl/README.md ]; then echo "Error: no source tree in ./src/vdp/userspace-vdp."; echo "Maybe you forgot to run: git submodule update --init"; echo; exit 1; fi
endif

vdp:
ifeq ($(PLATFORM),mac)
	echo "ON MAC!"
	EXTRA_FLAGS=-Wc++11-narrowing $(MAKE) -C src/vdp
else
	$(MAKE) -C src/vdp
endif
	cp src/vdp/*.so firmware/

cargo:
ifeq ($(OS),Windows_NT)
	set FORCE=1 && cargo build -r
	cp ./target/release/fab-agon-emulator.exe .
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
