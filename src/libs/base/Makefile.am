##### Add additional distribution files here!
EXTRA_DIST = 

lib_LIBRARIES = libbase.a

##### Add additional include directories here!
INCLUDES = -I@LOCAL_INC_DIR@ -I@KERNEL_INC_DIR@ 

##### Add the source files to be build here!

if CS8900A_ETHERNET
NET_SRC = ethernet_cs8900a.c

else

if SMSC91C111_ETHERNET
NET_SRC = ethernet_smsc91c111.c eeprom_24lc64.c

else

# No network driver was specified--will cause a link error eventually
NET_SRC =

endif
endif

if XSCALE
CPU_SRC = serial_xscale.c i2c_xscale.c timer_xscale.c
else
CPU_SRC = serial_sa.c timer_sa.c
endif

if TAGGED_EEPROM
EEPROM_SRC = eeprom.c nvconfig.c
else
EEPROM_SRC =
endif

libbase_a_SOURCES = $(CPU_SRC) $(NET_SRC) $(EEPROM_SRC) \
                    cpu.c flash_i128.c help.c idle.c os.c params.c serial.c pcmcia.c
