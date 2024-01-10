/*
 * file: testpreempt.c
 */
#include <8051.h>
#include "preemptive.h"

__data __at (0x3A) char head;
__data __at (0x3B) char tail;
__data __at (0x3C) char shared_buff[3];
__data __at(0x38) char buffer;
__data __at (0x4D) Semaphore empty;
__data __at (0x4E) Semaphore mutex;
__data __at (0x4F) Semaphore full;


/* [8 pts] for this function
 * the producer in this test program generates one characters at a
 * time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        buffer = 'B';
        while (1) {
                /* @@@ [6 pt]
                * wait for the buffer to be available, 
                * and then write the new data into the buffer
                */
                // if(buff_avail){
                        // EA = 0;
                SemaphoreWait(empty);
                SemaphoreWait(mutex);
                        __critical{
                                shared_buff[head] = buffer + 'A';
                                head = (head + 1) %3;
                                buffer = (buffer + 1 + 'A') % 26;
                        }
                        // EA = 1;
                // }
                // ThreadYield();
                SemaphoreSignal(mutex);
                SemaphoreSignal(full);
        }       
}

/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void) {
        /* @@@ [2 pt] initialize Tx for polling */
        TMOD |= 0x20;
        TH1 = (char)-6;
        SCON = 0x50;
        TR1 = 1;
        TI = 1;

        while (1) {
                /* @@@ [2 pt] wait for new data from producer
                * @@@ [6 pt] write data to serial port Tx, 
                * poll for Tx to finish writing (TI),
                * then clear the flag
                */
                // if(buff_avail == 0){
                SemaphoreWait(full);
                SemaphoreWait(mutex);
                        // EA = 0;
                        while(TI == 0){};
                        __critical{
                                SBUF = shared_buff[tail];
                                TI = 0;
                                tail = (tail + 1) %3;
                        }
                        // EA = 1;
                        // ThreadYield();
                // }
                SemaphoreSignal(mutex);
                SemaphoreSignal(empty);
                // else ThreadYield();
        }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can act as producer and another as consumer.
 */
void main(void) {
          /* 
           * @@@ [1 pt] initialize globals 
           * @@@ [4 pt] set up Producer and Consumer.
           * Because both are infinite loops, there is no loop
           * in this function and no return.
           */
        
        // shared_bufff = 'A';
        // buff_avail = 0;
        head = tail = 0;
        shared_buff[0] = shared_buff[1] = shared_buff[2] = ' ';
        SemaphoreCreate(empty, 3);
        SemaphoreCreate(mutex, 1);
        SemaphoreCreate(full, 0);
        ThreadCreate(Producer);
        // cur_thr_id = ThreadCreate(Producer);
        // __asm
        Consumer();         
}

void _sdcc_gsinit_startup(void) {
        __asm
                ljmp  _Bootstrap
        __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}

void timer0_ISR(void) __interrupt(1) {
        __asm
                ljmp _myTimer0Handler
        __endasm;
}
