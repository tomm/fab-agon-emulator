all:
	$(MAKE) -C userspace-vdp
	cp userspace-vdp/*.so .

clean:
	$(MAKE) -C userspace-vdp clean

depends:
	$(MAKE) -C userspace-vdp depends
