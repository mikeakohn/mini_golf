
PROGRAM=pn532
INCLUDE= \
  -I../naken_asm/include/msp430 \
  -I../naken_asm/include

default:
	naken_asm -l -o $(PROGRAM).hex $(INCLUDE) $(PROGRAM).asm

clean:
	@rm -f *.hex *.lst
	@echo "Clean!"

