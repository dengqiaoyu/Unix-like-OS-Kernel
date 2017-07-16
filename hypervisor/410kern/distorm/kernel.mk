410KLIB_DISTORM_OBJS := \
						decoder.o \
						distorm.o \
						instructions.o \
						insts.o \
						operands.o \
						prefix.o \
						textdefs.o \
						wstring.o \
						x86defs.o \
						\
						disassemble.o \

410KLIB_DISTORM_OBJS := $(410KLIB_DISTORM_OBJS:%=$(410KDIR)/distorm/%)

ALL_410KOBJS += $(410KLIB_DISTORM_OBJS)
410KCLEANS += $(410KDIR)/libdistorm.a

$(410KDIR)/libdistorm.a: $(410KLIB_DISTORM_OBJS)

