
PROGRAM=golf_box
ATMEL_INCLUDES=

default: $(PROGRAM).hex

$(PROGRAM).hex: $(PROGRAM).asm
	naken_asm -l -I$(ATMEL_INCLUDES) -o $(PROGRAM).hex $(PROGRAM).asm

program:
	avrdude -c usbtiny -p t85 -b19200 -v -U flash:w:$(PROGRAM).hex

setfuse:
	avrdude -c usbtiny -p t85 -b19200 -v -U lfuse:w:0xe1:m

clean:
	@rm -f *.hex
	@rm -f *.lst
	@echo "Clean!"

