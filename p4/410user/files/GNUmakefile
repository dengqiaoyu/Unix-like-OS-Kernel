PROGS=spin1 spin2 hi vr byenow

# cancel implicit rule to build an executable from a .S with cc
%: %.S

%.o: %.S
	cc -m32 -DASSEMBLER -c $<

%: %.o
	ld -T kernel.lds -static -Ttext 100000 --fatal-warnings -melf_i386 -o $@ $<

# "don't delete intermediates"
# .SECONDARY:

.PHONY: all
all: $(PROGS)

clean:
	rm -f $(PROGS)
