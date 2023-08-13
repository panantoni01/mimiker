TOPDIR = $(realpath ../..)

SRC = st.c x.c
HDR = arg.h st.h win.h

PROGRAM = sbase-box
BINDIR = bin
BUILDDIR = build/
SOURCES = $(addprefix st/,$(SRC))
FORMAT-EXCLUDE = $(addprefix st/,$(SRC)) $(addprefix st/,$(HDR))

$(OBJECTS): $(BUILDDIR)st/config.h

$(BUILDDIR)%.h: %.def.h
	cp $< $@

CPPFLAGS += -I$(BUILDDIR)/st

include $(TOPDIR)/build/build.prog.mk

download-here: st/.git

build-before: quilt-patch

clean-here: quilt-unpatch
