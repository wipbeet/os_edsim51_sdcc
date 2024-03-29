#include <8051.h>

#include "preemptive.h"

/*
 * @@@ [2 pts] declare the static globals here using 
 *        __data __at (address) type name; syntax
 * manually allocate the addresses of these variables, for
 * - saved stack pointers (MAXTHREADS)
 * - current thread ID
 * - a bitmap for which thread ID is a valid thread; 
 *   maybe also a count, but strictly speaking not necessary
 * - plus any temporaries that you need.
 */
__data __at (0x30) char ssp[MAXTHREADS];
__data __at(0x35) ThreadID cur_thr;
__data __at (0x34) int bitmap;
__data __at (0x6C) char starting_stack;
__data __at (0x22) char tmp;
__data __at (0x6D) char turn; //1 for prod, 0 for consumer
__data __at (0x6E) char prev_turn;
__data __at (0x23) int next_char;
__data __at(0x49) ThreadID new_thr_id;

/*
 * @@@ [8 pts]
 * define a macro for saving the context of the current thread by
 * 1) push ACC, B register, Data pointer registers (DPL, DPH), PSW
 * 2) save SP into the saved Stack Pointers array
 *   as indexed by the current thread ID.
 * Note that 1) should be written in assembly, 
 *     while 2) can be written in either assembly or C
 */
#define SAVESTATE \
        { __asm \
            push ACC \
            push B \
            push DPL \
            push DPH \
            push PSW \
         __endasm; \
        } \
        { __asm \
            mov a, r0 \
            push a \
            mov a, _cur_thr \
            add a, #_ssp \
            mov r0, a \
            mov @r0, _SP \
            pop a \
            mov r0, a \
         __endasm; \
        }

/*
 * @@@ [8 pts]
 * define a macro for restoring the context of the current thread by
 * essentially doing the reverse of SAVESTATE:
 * 1) assign SP to the saved SP from the saved stack pointer array
 * 2) pop the registers PSW, data pointer registers, B reg, and ACC
 * Again, popping must be done in assembly but restoring SP can be
 * done in either C or assembly.
 */
#define RESTORESTATE \
         { __asm \
            mov a, r1 \
            push a \
            mov a,_cur_thr \
            add a,#_ssp \
            mov r1,a \
            mov _SP,@r1 \
            pop a \
            mov r1, a \
         __endasm; \
         } \
         { __asm \
            pop PSW \
            pop DPH \
            pop DPL \
            pop B \
            pop ACC \
            __endasm; \
         }


 /* 
  * we declare main() as an extern so we can reference its symbol
  * when creating a thread for it.
  */

extern void main(void);

/*
 * Bootstrap is jumped to by the startup code to make the thread for
 * main, and restore its context so the thread can run.
 */

void Bootstrap(void) {
      /*
       * @@@ [2 pts] 
       * initialize data structures for threads (e.g., mask)
       *
       * optional: move the stack pointer to some known location
       * only during bootstrapping. by default, SP is 0x07.
       *
       * @@@ [2 pts]
       *     create a thread for main; be sure current thread is
       *     set to this thread ID, and restore its context,
       *     so that it starts running main().
       */
      // turn = 1;
      bitmap = 0x0;
      TMOD = 0;  // timer 0 mode 0, meaning set timer 0 and timer 1 to 0
      IE = 0x82;  // enable timer 0 interrupt; keep consumer polling
                  // EA  -  ET2  ES  ET1  EX1  ET0  EX0
                  // meaning = set all interrupt and set timer interrupt
      TR0 = 1; // set bit TR0 to start running timer 0, meaning start timer 0
      cur_thr = ThreadCreate(main);
      RESTORESTATE;
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {
   // __critical{
        /*
         * @@@ [2 pts] 
         * check to see we have not reached the max #threads.
         * if so, return -1, which is not a valid thread ID.
         */
        EA = 0;
        if (bitmap == (1 << MAXTHREADS) - 1) return -1;
        /*
         * @@@ [5 pts]
         *     otherwise, find a thread ID that is not in use,
         *     and grab it. (can check the bit mask for threads),
         */
        for (new_thr_id = 0; new_thr_id < MAXTHREADS; new_thr_id++) {
            __asm
               push 6
               push 7
            __endasm;
            next_char = bitmap & (1 << new_thr_id);
            __asm
               pop 7
               pop 6
            __endasm;
            if (next_char) continue;
            break;
        }
         /* @@@ [18 pts] below
         * a. update the bit mask 
             (and increment thread count, if you use a thread count, 
              but it is optional)
           b. calculate the starting stack location for new thread
           c. save the current SP in a temporary
              set SP to the starting location for the new thread
           d. push the return address fp (2-byte parameter to
              ThreadCreate) onto stack so it can be the return
              address to resume the thread. Note that in SDCC
              convention, 2-byte ptr is passed in DPTR.  but
              push instruction can only push it as two separate
              registers, DPL and DPH.
           e. we want to initialize the registers to 0, so we
              assign a register to 0 and push it four times
              for ACC, B, DPL, DPH.  Note: push #0 will not work
              because push takes only direct address as its operand,
              but it does not take an immediate (literal) operand.
           f. finally, we need to push PSW (processor status word)
              register, which consist of bits
               CY AC F0 RS1 RS0 OV UD P
              all bits can be initialized to zero, except <RS1:RS0>
              which selects the register bank.  
              Thread 0 uses bank 0, Thread 1 uses bank 1, etc.
              Setting the bits to 00B, 01B, 10B, 11B will select 
              the register bank so no need to push/pop registers
              R0-R7.  So, set PSW to 
              00000000B for thread 0, 00001000B for thread 1,
              00010000B for thread 2, 00011000B for thread 3.
           g. write the current stack pointer to the saved stack
              pointer array for this newly created thread ID
           h. set SP to the saved SP in step c.
           i. finally, return the newly created thread ID.
         */
         bitmap |= (1 << new_thr_id);//a

         starting_stack = ((new_thr_id+3) << 4) | 0x0F;//b

         tmp = SP; //c
         SP = starting_stack;

         // next_char = 'A';
         next_char = new_thr_id << 3;
         //d & e
         __asm
                push DPL 
                push DPH
                MOV A, #0x0
                push A 
                push A
                push A
                push A
                push _next_char
        __endasm;

        __asm //f
            mov a, r0 
            push a 
            mov a, _new_thr_id 
            add a, #_ssp
            mov r0, a 
            mov @r0, _SP 
            pop a
            mov r0, a
         __endasm; 

         SP = tmp; //h
         EA = 1;
         return new_thr_id; //i
   // }

}



/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
   __critical{
       SAVESTATE;
       do {
                /*
                 * @@@ [8 pts] do round-robin policy for now.
                 * find the next thread that can run and 
                 * set the current thread ID to it,
                 * so that it can be restored (by the last line of 
                 * this function).
                 * there should be at least one thread, so this loop
                 * will always terminate.
                 */
                cur_thr = (cur_thr + 1) % MAXTHREADS;
                __asm
                  push 6
                  push 7
               __endasm;
               next_char = bitmap & (1 << cur_thr);
               __asm
                  pop 7
                  pop 6
               __endasm;
               if (next_char)
                  break;

        } while (1);
        RESTORESTATE;
   }
}


/*
 * ThreadExit() is called by the thread's own code to terminate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
   __critical{
        /*
         * clear the bit for the current thread from the
         * bit mask, decrement thread count (if any),
         * and set current thread to another valid ID.
         * Q: What happens if there are no more valid threads?
         */
        bitmap &= ~(1 << cur_thr);
        ThreadYield();
        RESTORESTATE;
   }
}

void myTimer0Handler(){
   // __critical{
      EA = 0;
      SAVESTATE;
      do {
         /*
         * @@@ [8 pts] do round-robin policy for now.
         * find the next thread that can run and 
         * set the current thread ID to it,
         * so that it can be restored (by the last line of 
         * this function).
         * there should be at least one thread, so this loop
         * will always terminate.
         */
         // cur_thr = (cur_thr + 1) % MAXTHREADS;
         if(turn){
            cur_thr = turn;
            prev_turn = turn;
            turn = 0;
         }
         else{
            cur_thr = 0;
            turn = (prev_turn == 1) ? 2 : 1;
         }
         __asm
            push 6
            push 7
         __endasm;
         next_char = bitmap & (1 << cur_thr);
         __asm
            pop 7
            pop 6
         __endasm;
         if (next_char)
            break;
      } while (1);
      RESTORESTATE;
   // }
   EA = 1;
   //RETI assembly instruction
   __asm
      RETI
   __endasm;

}