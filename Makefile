# I-Boot Lite root makefile
# See the file documentation/INSTALL for details

ROOT_DIR = $(PWD)

export CC=arm-linux-gcc

export MAJOR=1
export MINOR=7
ifeq ($(BUILD),)
#BUILD=0
export BUILD
endif

ifeq ($(BUILD),)
export VERSION=$(MAJOR).$(MINOR)
else
export VERSION=$(MAJOR).$(MINOR).$(BUILD)
endif

# List all targets to build here
all: \
	i-boot-cerf-board-pxa250

# The *.ipk targets make the ipkg files so you can upgrade the bootloader
# while Linux is running
ipkg: \
	i-boot-cerf-board-pxa250.ipk


# Following are the configuration targets for each hardware platform for which
# we can build I-Boot Lite.  To add support for a new platform, just add
# an entry here with the appropriate configure options and append it to the
# all: section above.

build-i-boot-cerf-board-pxa250/Makefile: src/configure
	test -d $(@D) || mkdir -p $(@D)
	(cd $(@D); "$(ROOT_DIR)/src/configure" --target=arm-intrinsyc \
		--enable-xscale \
		--enable-interleaved \
		--enable-smsc91c111 \
		--with-ldscript=ld-xscale \
		--with-ram=64 \
	)

# Internal make stuff follow

CLEAN_LIST =	aclocal.m4 configure ltmain.sh mkinstalldirs \
		config.guess install-sh config.h.in stamp-h.in \
		config.sub ltconfig missing
FILE_LIST =	documentation README src CHANGE-NOTES COPYING \
		Makefile RELEASE-NOTES

src/configure: src/configure.in
	(cd "$(ROOT_DIR)/src"; $(MAKE) -f Makefile.dist)

i-boot-lite:
	mkdir -p "$(ROOT_DIR)/i-boot-lite"

i-boot-%: build-i-boot-%/Makefile i-boot-lite
	(cd $(<D); $(MAKE) all)
	cp $(<D)/main/i_boot i-boot-lite/$@_$(VERSION)-lite.img

src-dist:
	$(MAKE) distclean
	mkdir -p i-boot-lite-$(VERSION).dist
	mkdir -p "$(ROOT_DIR)/i-boot-lite-$(VERSION).src"
	for file in $(FILE_LIST); do \
		find "$$file" -path '*/SCCS' -prune -o -type f -exec cp --parents -a {} "$(ROOT_DIR)/i-boot-lite-$(VERSION).src" \; ; \
	done
	tar -cvf "$(ROOT_DIR)/i-boot-lite-$(VERSION).dist/i-boot-lite-$(VERSION).src.tar" \
		i-boot-lite-$(VERSION).src
	gzip "$(ROOT_DIR)/i-boot-lite-$(VERSION).dist/i-boot-lite-$(VERSION).src.tar"
	rm -rf "$(ROOT_DIR)/i-boot-lite-$(VERSION).src"

binary-dist:
	$(MAKE) all
	$(MAKE) ipkg
	rm -rf "i-boot-lite/ipkg"
	mkdir -p i-boot-lite-$(VERSION).dist
	tar -cvf "$(ROOT_DIR)/i-boot-lite-$(VERSION).dist/i-boot-lite-$(VERSION).bin.tar" \
		i-boot-lite
	gzip "$(ROOT_DIR)/i-boot-lite-$(VERSION).dist/i-boot-lite-$(VERSION).bin.tar"

dist: src-dist binary-dist

clean:
	rm -rf "$(ROOT_DIR)/i-boot-lite"
	rm -rf "$(ROOT_DIR)/i-boot-lite-$(VERSION).src"
	rm -rf "$(ROOT_DIR)/i-boot-lite-$(VERSION).dist"
	rm -rf "$(ROOT_DIR)/build-"*
	rm -f "$(ROOT_DIR)/i-boot"*.ipk

distclean: clean
	find . -name "Makefile.in" -print -exec rm -f '{}' \;
	cd "$(ROOT_DIR)/src" ; \
		for file in $(CLEAN_LIST); do \
			rm -f "$$file"; \
		done

i-boot-%.ipk: i-boot-%	
	mkdir -p "$(ROOT_DIR)/i-boot-lite/ipkg"
	PROCESSOR=`echo $< | awk -F "-" '{print $$5}' | sed "s/\([a-z][a-z]*\)\([0-9][0-9]*\)/\1-\2/" | sed "y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/"`\
	DEVICE=`echo $< | awk -F "-" '{print $$4}'`\
	FAMILY=`echo $< | awk -F "-" '{print $$3}'` \
	IPK_NAME=$@ ; \
	for file in control rules postinst; do \
		sed s/@pkgname@/$</ "$(ROOT_DIR)/src/ipkg-templates/$$file" | \
		    sed s/@processor@/$$PROCESSOR/ | \
			sed s/@family@/$$FAMILY/ | \
			sed s/@device@/$$DEVICE/ | \
			sed s/@version@/$(VERSION)/ > \
			"$(ROOT_DIR)/i-boot-lite/ipkg/$$file"; \
	done
	for file in rules postinst; do \
		chmod +x "$(ROOT_DIR)/i-boot-lite/ipkg/$$file" ; \
	done
	cd "$(ROOT_DIR)/i-boot-lite" ; \
	fakeroot -- ipkg-buildpackage
