;; PN532 RFID reader.
;;
;; Copyright 2024 - By Michael Kohn
;; https://www.mikekohn.net/
;; mike to mikekohn.net
;;
;; Sample of reading from an RFID tag with a PN532 based reader.

.include "msp430x2xx.inc"
.include "rfid/pn532.inc"

;; I/O Port 1.
SPI_CS   equ 0x10
SPI_CLK  equ 0x20
SPI_SOMI equ 0x40
SPI_SIMO equ 0x80

;; I/O Port 2.
RFID_IRQ equ 0x01
RFID_RST equ 0x02
LED0     equ 0x08
LED1     equ 0x10
LED2     equ 0x20

RAM equ 0x200
PACKET equ RAM
MAX_PACKET equ PACKET + 256

.org 0xc000
start:
  ;; Turn off watchdog.
  mov.w #WDTPW|WDTHOLD, &WDTCTL

  ;; Turn interrupts off.
  dint

  ;; Setup stack pointer.
  mov.w #0x0400, SP

  ;; Set MCLK to 1.4 MHz with DCO.
  mov.b #DCO_3, &DCOCTL
  mov.b #RSEL_8, &BCSCTL1
  mov.b #0, &BCSCTL2

  ;; Setup output pins.
  ;; P1.4 = SPI /SEN (chip select)
  ;; P1.5 = SPI CLK
  ;; P1.6 = SPI SOMI
  ;; P1.7 = SPI SIMO
  ;; P2.0 = RFID_IRQ
  ;; P2.1 = RFID_RST
  ;; P2.3 = LED0
  ;; P2.4 = LED1
  ;; P2.5 = LED2
  mov.b #SPI_CS|SPI_CLK|SPI_SIMO, &P1DIR
  mov.b #SPI_CS, &P1OUT
  mov.b #SPI_CLK|SPI_SOMI|SPI_SIMO, &P1SEL
  mov.b #SPI_CLK|SPI_SOMI|SPI_SIMO, &P1SEL2
  mov.b #RFID_RST|LED0|LED1|LED2, &P2DIR
  mov.b #0, &P2OUT

  ;; Setup SPI.
  ;; MSB first, Master, 3 pin SPI, Sync mode
  mov.b #UCSWRST, &UCB0CTL1      ; Set reset
  bis.b #UCSSEL_2, &UCB0CTL1     ; SMCLK
  ;mov.b #0|UCMSB|UCMST|UCSYNC, &UCB0CTL0
  ;mov.b #UCCKPH|UCMSB|UCMST|UCSYNC, &UCB0CTL0
  mov.b #UCCKPH|UCMST|UCSYNC, &UCB0CTL0
  ;mov.b #UCMST|UCSYNC, &UCB0CTL0
  ;mov.b #UCCKPL|UCMST|UCSYNC, &UCB0CTL0
  mov.b #1, &UCB0BR0
  mov.b #0, &UCB0BR1
  bic.b #UCSWRST, &UCB0CTL1      ; Clear reset

  eint

  mov.w #RAM, r12
memset:
  mov.b #0, 0(r12)
  add.w #1, r12
  sub.w #1, r14
  jnz memset

  call #delay

  ;; Set high to take out of reset mode.
  bis.b #RFID_RST, &P2OUT

  ;; A debug register.
  mov.w #0, r8

  call #init_rfid

main:
  mov.w #packet_get_firmware_version, r14
  call #send_and_get_response

  mov.w #packet_rf_configuration_rfon, r14
  call #send_and_get_response

  mov.w #packet_rf_configuration_retries, r14
  call #send_and_get_response

  ;call #delay
  ;call #delay
  ;call #delay
  ;call #delay
  ;call #delay
  ;call #delay
  ;call #delay
  ;call #delay

  ;mov.w #packet_get_data, r14
  ;call #send_and_get_response

main_while_1:
  bic.b #LED0|LED1|LED2, &P2OUT

  call #delay_short
  mov.w #packet_in_list_passive_target, r14
  call #transmit_packet
  call #wait_for_irq
  call #receive_ack

  cmp.w #1, r15
  jz main_while_1

  bic.b #SPI_CS, &P1OUT
  call #spi_wait_ready
  bis.b #SPI_CS, &P1OUT

  ;call #wait_for_irq
  call #receive_packet

  cmp.b #0x1e, &PACKET+0x11
  jnz main_not_0
  bis.b #LED0, &P2OUT
  call #delay_long
  jmp main_while_1
main_not_0:

  cmp.b #0xd8, &PACKET+0x11
  jnz main_not_1
  bis.b #LED1, &P2OUT
  call #delay_long
  jmp main_while_1
main_not_1:

  cmp.b #0x4f, &PACKET+0x11
  jnz main_not_2
  bis.b #LED2, &P2OUT
  call #delay_long
  jmp main_while_1
main_not_2:

  bis.b #LED0, &P2OUT
  bis.b #LED1, &P2OUT
  bis.b #LED2, &P2OUT
  call #delay_long
  jmp main_while_1

;; init_rfid()
init_rfid:
  ;; Hold CS low to take it out of low BAT mode into normal mode.
  call #delay
  bic.b #SPI_CS, &P1OUT
  call #delay_short
  ;bis.b #SPI_CS, &P1OUT
init_rfid_try_again:
  mov.w #packet_sam_config, r14
  call #transmit_packet
  call #wait_for_irq_with_timeout
  cmp.w #0, r15
  jnz init_rfid_try_again
  call #receive_ack
  call #wait_for_irq
  call #receive_packet
  ret

send_and_get_response:
  call #transmit_packet
  call #wait_for_irq
  call #receive_ack
  call #wait_for_irq
  call #receive_packet
  ret

;; wait_for_irq()
wait_for_irq:
  bit.b #RFID_IRQ, &P2IN
  jnz wait_for_irq
  ret

;; wait_for_irq_with_timeout()
wait_for_irq_with_timeout:
  mov.w #9000, r9
wait_for_irq_with_timeout_loop:
  dec.w r9
  jz wait_for_irq_with_timeout_loop_exit
  bit.b #RFID_IRQ, &P2IN
  jnz wait_for_irq_with_timeout_loop
  mov.w #0, r15
  ret
wait_for_irq_with_timeout_loop_exit:
  mov.w #1, r15
  ret

;; transmit_packet(r14)
transmit_packet:
  call #copy_packet_to_ram
  ;; Send packet.
  mov.w #PACKET, r14
  bic.b #SPI_CS, &P1OUT
  ;; 6.2.5 (page 45) in the documentation explains this extra byte for SPI.
  call #spi_data_writing
  mov.b &PACKET+3, r13
  add.w #7, r13
transmit_packet_send_loop:
  mov.b @r14+, r15
  call #send_spi
  dec.w r13
  jnz transmit_packet_send_loop
  bis.b #SPI_CS, &P1OUT
  ret

spi_status_reading:
  mov.b #0x02, r15
  call #send_spi
  call #read_spi
  ret

spi_data_writing:
  mov.b #0x01, r15
  call #send_spi
  ret

spi_data_reading:
  mov.b #0x03, r15
  call #send_spi
  ret

;; spi_wait_ready()
spi_wait_ready:
  mov.b #0x02, r15
  call #send_spi
spi_wait_ready_loop:
  call #read_spi
  bit.b #1, r15
  jz spi_wait_ready_loop
  ret

;; receive_packet()
receive_packet:
  mov.w #PACKET, r14
  bic.b #SPI_CS, &P1OUT
  ;call #spi_wait_ready
  call #spi_data_reading
  ;; Read 00 00 FF LEN.
  call #read_spi
  mov.b r15, 0(r14)
  call #read_spi
  mov.b r15, 1(r14)
  call #read_spi
  mov.b r15, 2(r14)
  call #read_spi
  mov.b r15, 3(r14)
  ;; r14 = PACKET + 4, r13 = LEN + 3 (LCS, DCS, and 00 isn't a part of LEN).
  mov.b 3(r14), r13
  add.w #4, r14
  add.w #3, r13
  ;; Read the rest of the packet.
receive_packet_next:
  call #read_spi
  mov.b r15, 0(r14)
  inc.w r14
  dec.w r13
  jnz receive_packet_next
receive_packet_exit:
  bis.b #SPI_CS, &P1OUT
  ret

;; receive_packet()
receive_packet_2:
  mov.w #PACKET, r14
  bic.b #SPI_CS, &P1OUT
  ;call #spi_wait_ready
  ;call #spi_data_reading
  ;; Read 00 00 FF LEN.
  call #read_spi
  mov.b r15, 0(r14)
  call #read_spi
  mov.b r15, 1(r14)
  call #read_spi
  mov.b r15, 2(r14)
  call #read_spi
  mov.b r15, 3(r14)
  ;; r14 = PACKET + 4, r13 = LEN + 3 (LCS, DCS, and 00 isn't a part of LEN).
  mov.b 3(r14), r13
  add.w #4, r14
  add.w #3, r13
  ;; Read the rest of the packet.
receive_packet_2_next:
  call #read_spi
  mov.b r15, 0(r14)
  inc.w r14
  dec.w r13
  jnz receive_packet_2_next
receive_packet_2_exit:
  bis.b #SPI_CS, &P1OUT
  ret

;; receive_ack() (6.2.1.3 page 30 in documentation)
receive_ack:
  mov.w #PACKET, r14
  bic.b #SPI_CS, &P1OUT
  ;call #spi_wait_ready
  call #spi_data_reading
  mov.w #6, r13
receive_ack_loop:
  call #read_spi
  mov.b r15, 0(r14)
  add.w #1, r14
  dec.w r13
  jnz receive_ack_loop
  cmp.b #0x00, &PACKET+3
  jz receive_ack_is_good
  call #read_spi
  mov.b r15, 0(r14)
  call #read_spi
  mov.b r15, 1(r14)
  call #read_spi
  mov.b r15, 2(r14)
receive_ack_is_good:
  bis.b #SPI_CS, &P1OUT
  mov.b &PACKET+3, r15
  ret

;; transmit_ack() (6.2.1.3 page 30 in documentation)
transmit_ack:
  mov.w #packet_ack, r14
  bic.b #SPI_CS, &P1OUT
  call #spi_data_writing
  mov.w #6, r13
transmit_ack_loop:
  mov.b @r14+, r15
  call #send_spi
  dec.w r13
  jnz transmit_ack_loop
  bis.b #SPI_CS, &P1OUT
  ret

;; copy_packet_to_ram(r14=source)
copy_packet_to_ram:
  mov.b 3(r14), r13
  add.w #7, r13
  mov.w #PACKET, r15
copy_packet_to_ram_loop:
  mov.b @r14+, @r15
  add #1, r15
  dec r13
  jnz copy_packet_to_ram_loop
  call #update_checksum
  ret

;; calculate_len_checksum() : r15
calculate_len_checksum:
  mov.b &PACKET+3, r15
  xor.b #0xff, r15
  add.b #1, r15
  ret

;; calculate_data_checksum() : r15
;; Length of packet cannot be 0.
calculate_data_checksum:
  mov.w #PACKET+5, r13
  mov.b &PACKET+3, r14
  mov.b #0, r15
calculate_data_checksum_loop:
  add.b @r13+, r15
  dec.b r14
  jnz calculate_data_checksum_loop
  xor.b #0xff, r15
  add.b #1, r15
  ret

;; update_checksum()
update_checksum:
  call #calculate_len_checksum
  mov.b r15, &PACKET+4
  call #calculate_data_checksum
  mov.b &PACKET+3, r13
  mov.w #PACKET+5, r14
  add.w r13, r14
  mov.b r15, @r14
  ret

;; send_spi(r15) : r15
send_spi:
  mov.b r15, &UCB0TXBUF
send_spi_wait:
  bit.b #UCB0RXIFG, &IFG2
  jz send_spi_wait
  mov.b &UCB0RXBUF, r15
  ret

;; read_spi() : r15
read_spi:
  mov.b #0, &UCB0TXBUF
read_spi_wait:
  bit.b #UCB0RXIFG, &IFG2
  jz read_spi_wait
  mov.b &UCB0RXBUF, r15
  ret

;; send_spi_sw(r15) : r15 // Software version.
send_spi_sw:
  mov.w #8, r4
  mov.b #0, r5
send_spi_next_bit:
  bic.b #SPI_SIMO, &P1OUT
  bit.b #0x80, r15
  jz send_spi_out_not_1
  bis.b #SPI_SIMO, &P1OUT
send_spi_out_not_1:
  bis.b #SPI_CLK, &P1OUT
  rla.w r15
  rla.w r5
  bit.b #SPI_SOMI, &P1IN
  jz send_spi_in_not_1
  bis.b #1, r5
send_spi_in_not_1:
  bic.b #SPI_CLK, &P1OUT
  sub.w #1, r4
  jnz send_spi_next_bit
  bic.b #SPI_SIMO, &P1OUT
  mov.b r5, r15
  ret

;; delay()
;; Around 196605 * 5 cycles.
;; @1.4MHz this should be around 702ms.
delay:
  mov.w #5, r14
delay_loop_outer:
  mov.w #0xffff, r15
delay_loop:
  dec.w r15
  jnz delay_loop
  dec.w r14
  jnz delay_loop_outer
  ret

;; delay()
;; Around 196605 * 8 cycles.
;; @1.4MHz this should be around 1.1ms.
delay_long:
  mov.w #8, r14
delay_long_loop_outer:
  mov.w #0xffff, r15
delay_long_loop:
  dec.w r15
  jnz delay_long_loop
  dec.w r14
  jnz delay_long_loop_outer
  ret

;; delay_short()
;; Around 196605 cycles.
;; @1.4MHz this should be around 140ms.
delay_short:
  mov.w #0xffff, r15
delay_short_loop:
  dec.w r15
  jnz delay_short_loop
  ret

;; Packets are: 00 00 ff LEN LCS TFI PD0 PD1 ... PDn DCS 00
;; LEN is: Count of TFI and PD0 to PDn.
;; LCS is: Checksum [LEN + LCS] = 0x00.
;; TFI is:
;;   0xd4 Host to RFID.
;;   0xd5 RFID to host.
;; DCS is: Checksum [TFI + PD0 + PD1 + ... + PDn] = 0x00;

.define lcs 0
.define dcs 0

.align 16
packet_sam_config:
  .db 0x00, 0x00, 0xff
  .db    5, lcs
  .db 0xd4, PN532_CMD_SAM_CONFIGURATION, 0x01, 0x14, 0x01
  .db dcs,  0x00

.align 16
packet_get_firmware_version:
  .db 0x00, 0x00, 0xff
  .db    2, lcs
  .db 0xd4, PN532_CMD_GET_FIRMWARE_VERSION
  .db dcs,  0x00
  .db 0

.align 16
packet_rf_configuration_rfon:
  .db 0x00, 0x00, 0xff
  .db    4, lcs
  .db 0xd4, PN532_CMD_RF_CONFIGURATION, 0x01, 0x03
  .db dcs,  0x00
  .db 0

.align 16
packet_rf_configuration_retries:
  .db 0x00, 0x00, 0xff
  .db    6, lcs
  .db 0xd4, PN532_CMD_RF_CONFIGURATION, 0x05, 0xff, 0x01, 0xff
  .db dcs,  0x00
  .db 0

.align 16
packet_in_list_passive_target:
  .db 0x00, 0x00, 0xff
  .db    4, lcs
  .db 0xd4, PN532_CMD_IN_LIST_PASSIVE_TARGET, 0x01, 0x00
  .db dcs,  0x00
  .db 0

.align 16
packet_get_data:
  .db 0x00, 0x00, 0xff
  .db    4, lcs
  .db 0xd4, PN532_CMD_TG_GET_DATA, 0x86
  .db dcs,  0x00
  .db 0

;; ACK can be sent from host to PN532 or from PN532 to host.
.align 16
packet_ack:
  .db 0x00, 0x00, 0xff, 0x00, 0x0ff, 0x00

;; NACK is only sent from host to PN532 to indicate transmission error
;; and request retransmit.
.align 16
packet_nack:
  .db 0x00, 0x00, 0xff, 0xff, 0x000, 0x00

;; ERROR is sent from PN532 to host to indicate an error was detected.
.align 16
packet_error:
  .db 0x00, 0x00, 0xff, 0x01, 0xff, 0x07f, 0x81, 0x00

.org 0xfffe
  dw start                 ; Reset

