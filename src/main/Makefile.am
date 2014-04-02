# Makefile for I-Boot Lite main

bin_PROGRAMS = i_boot_elf
# This target doesn't need to be linked
bin_SCRIPTS = i_boot
CLEANFILES = i_boot

##### Build flags
INCLUDES = -I@LOCAL_INC_DIR@ -I@KERNEL_INC_DIR@

##### Add the source files to be built here!
if XSCALE
CPU_SRC = start_xscale.S 
else
CPU_SRC = start_sa.S 
endif

i_boot_elf_SOURCES = $(CPU_SRC) c_main.c 
i_boot_elf_LDFLAGS = -Wl,-T,$(srcdir)/../ldscripts/$(LDSCRIPT)

##### Add additional libraries used here!
i_boot_elf_LDADD = ../libs/tools/libtools.a ../libs/base/libbase.a ../libs/net/libnet.a -lgcc

##### Generate the binary image
i_boot: i_boot_elf
	$(OBJCOPY) $(OCFLAGS) $< $@
