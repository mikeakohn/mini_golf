;; Golf Box - Copyright 2025 by Michael Kohn
;; Email: mike@mikekohn.net
;;   Web: http://www.mikekohn.net/
;;
;; Obstacle for a mini golf course. Uses a servo motor to open and
;; close a door of a box. Two LEDs turn on while the box is open.

.include "tn85def.inc"
.avr8

; CKSEL = 0001
; CLKPS = 0000
; CKDIV8 = 0
;
; FUSE LOW = 11100001 = 0xe1

;  cycles  sample rate   @16 MHz:
;    210   38.0kHz * 2

; r0  = 0
; r1  = 1
; r15 = 255
; r14 = temp
; r17 = temp
; r18 = temp in main
; r20 = count in interrupt
; r21 = current byte being read in
; r22 = byte_count
; r23 = bit_count
; r24 =
; r25 = servo 0 position
; r26 =
; r27 =
; r28 = servo interrupt count low
; r29 = servo interrupt count high
; r30 = Z
; r31 = Z (pointer to IR data)
;

; note: CLKSEL 10

IR_DATA equ SRAM_START
SERVO_VALUE_0 equ IR_DATA + 0

;ROTATE_LOW    equ 100
ROTATE_LOW    equ 90
ROTATE_MIDDLE equ 114
;ROTATE_HIGH   equ 128
ROTATE_HIGH   equ 140

.org 0x000
  rjmp start
.org 0x00a
  rjmp service_interrupt

;; FIXME - erase this padding..
.org 0x020

start:
  ;; Disable interrupts
  cli

  ;; Set up stack ptr
  ;ldi r17, RAMEND>>8
  ;out SPH, r17
  ldi r17, RAMEND & 255
  out SPL, r17

  ;; r0 = 0, r1 = 1, r15 = 255
  eor r0, r0
  eor r1, r1
  inc r1
  ldi r17, 0xff
  mov r15, r17

  ;; Set up PORTB
  ;; PB0: LED 0
  ;; PB1: LED 1
  ;; PB4: Servo 0
  ldi r17, 0x13
  out DDRB, r17
  out PORTB, r0             ; turn off all of port B

  ;; Servo high value.
  ldi r28, 0xf0
  ldi r29, 0x05

  ldi r17, 240 - ROTATE_HIGH
  sts SERVO_VALUE_0, r17

  ;; Set up TIMER0
  ldi r17,  210                  ; set carrier freq
  out OCR0A, r17

  ldi r17, (1<<OCIE0A)
  out TIMSK, r17                 ; enable interrupt compare A
  ldi r17, (1<<WGM01)
  out TCCR0A, r17                ; normal counting (0xffff is top, count up)
  ldi r17, (1<<CS00)             ; CTC OCR0A  Clear Timer on Compare
  out TCCR0B, r17                ; prescale = 1 from clock source

  ; Enable interrupts
  sei

  rcall delay

main:
  cbi PORTB, 0
  cbi PORTB, 1

  ldi r18, 8
main_loop_closed:
  rcall delay
  rcall delay
  rcall delay
  dec r18
  brne main_loop_closed

  ldi r17, 240 - ROTATE_LOW
  sts SERVO_VALUE_0, r17

  sbi PORTB, 0
  sbi PORTB, 1

  ldi r18, 3
main_loop_open:
  rcall delay
  rcall delay
  rcall delay
  dec r18
  brne main_loop_open

  ldi r17, 240 - ROTATE_HIGH
  sts SERVO_VALUE_0, r17
  rjmp main

;; This appears to be 1/3 of a second.
;; 13us * 256 = 3.32ms
;; 3.32ms * 100 = 332ms
delay:
  ldi r17, 100
delay_repeat_outer:
  ldi r20, 0
delay_repeat:
  cpi r20, 255
  brne delay_repeat
  dec r17
  brne delay_repeat_outer
  ret

service_interrupt:
  in r7, SREG
  inc r20

  ;; Service servos
  sbiw r28, 1
  brne not_20ms
  ldi r28, 0xf0
  ldi r29, 0x05
  sbi PORTB, 4
  lds r25, SERVO_VALUE_0
not_20ms:
  cp r28, r25
  brne dont_update_servo_0
  cbi PORTB, 4
dont_update_servo_0:

exit_interrupt:
  out SREG, r7
  reti

