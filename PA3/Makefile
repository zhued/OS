CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

INPUTFILESIZEMEGABYTES = 1

KILO = 1024
MEGA = $(shell echo $(KILO)\*$(KILO) | bc)
INPUTFILESIZEBYTES = $(shell echo $(MEGA)\*$(INPUTFILESIZEMEGABYTES) | bc)
INPUTBLOCKSIZEBYTES = $(KILO)
INPUTBLOCKS = $(shell echo $(INPUTFILESIZEBYTES)\/$(INPUTBLOCKSIZEBYTES) | bc)

.PHONY: all clean

all: pi pi-sched rw rr_quantum CPUbound IObound MIXEDbound

pi: pi.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

pi-sched: pi-sched.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

rw: rw.o rwinput
	$(CC) $(LFLAGS) rw.o -o $@ -lm

rr_quantum: rr_quantum.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

CPUbound: CPUbound.o
	$(CC) $(LFLAGS) $^ -o $@ -lm
	
IObound: IObound.o
	$(CC) $(LFLAGS) $^ -o $@ -lm
	
MIXEDbound: MIXEDbound.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

pi.o: pi.c
	$(CC) $(CFLAGS) $<

pi-sched.o: pi-sched.c
	$(CC) $(CFLAGS) $<

rw.o: rw.c
	$(CC) $(CFLAGS) $<

rwinput: Makefile
	dd if=/dev/urandom of=./rwinput bs=$(INPUTBLOCKSIZEBYTES) count=$(INPUTBLOCKS)

rr_quantum.o: rr_quantum.c
	$(CC) $(CFLAGS) $<
	
CPUbound.o: CPUbound.c
	$(CC) $(CFLAGS) $<

IObound.o: IObound.c
	$(CC) $(CFLAGS) $<
	
MIXEDbound.o: MIXEDbound.c
	$(CC) $(CFLAGS) $<

clean: testclean
	rm -f pi pi-sched rw rr_quantum CPUbound IObound MIXEDbound
	rm -f rwinput
	rm -f *.o
	rm -f *~
	rm -f handout/*~
	rm -f handout/*.log
	rm -f handout/*.aux
	rm -f junk.txt

testclean:
	rm -f rwoutput*
