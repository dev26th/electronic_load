CONTROLLER=stm8s105k6
SDCC=sdcc
STM8FLASH=stm8flash

MAIN=main.c
SRC=system.c systemtimer.c strings.c flash.c uart.c displays.c button.c encoder.c encoderbutton.c beep.c fan.c load.c adc.c ringbuffer.c
RELS=$(SRC:.c=.rel)

all: bin

bin: $(MAIN) $(RELS)
	@mkdir -p bin
	$(SDCC) --std-c11 --opt-code-size -mstm8 -lstm8 $(MAIN) $(wildcard bin/*.rel) -o bin/

.c.rel:
	@mkdir -p bin
	$(SDCC) -c --std-c11 --opt-code-size -mstm8 -lstm8 $< -o bin/

clean:
	@rm -rf bin

#echo "0000ff00ff00ff00ff00ff00ff00ff" | xxd -r -p > misc/default_opt.bin
unlock:
	$(STM8FLASH) -c stlinkv2 -p $(CONTROLLER) -s opt -w misc/default_opt.bin
	#$(STM8FLASH) -c stlinkv2 -p $(CONTROLLER) -u # Works only in stm8flash versions after git commit f3f547b4

flash:
	$(STM8FLASH) -c stlinkv2 -p $(CONTROLLER) -w bin/main.ihx

eeprom:
	$(STM8FLASH) -c stlinkv2 -p $(CONTROLLER) -s eeprom -w misc/eeprom.bin

.SUFFIXES: .c .rel

.PHONY: clean flash

