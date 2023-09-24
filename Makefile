all:
	$(MAKE) -C userspace-vdp
	mkdir -p firmware
	cp userspace-vdp/*.so firmware/

clean:
	$(MAKE) -C userspace-vdp clean

depends:
	$(MAKE) -C userspace-vdp depends
