all:
	$(MAKE) -C src/vdp
	mkdir -p firmware
	cp src/vdp/*.so firmware/

clean:
	$(MAKE) -C src/vdp clean

depends:
	$(MAKE) -C src/vdp depends
