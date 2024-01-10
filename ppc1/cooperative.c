#include <8051.h>

#include "cooperative.h"
/*
savedSP[currentThread] r0, r1 to hold the address or offset.  

>> << ar7, ar6 (IDATA) => r7, r6 for bank-0
*/
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

__data __at (0x30) char savedSP[MAXTHREADS];
__data __at (0x34) int bitmap;
__data __at (0x35) ThreadID currentThread;

__data __at (0x20) ThreadID createdThread;
__data __at (0x21) char startStack;
__data __at (0x22) char oldSP;
__data __at (0x23) int tmp;


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
   /* savedSP[currentThread] = SP; */ \
   { __asm \
      mov a, r0 \
      push a \
      mov a, _currentThread \
      add a, #_savedSP \
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
   /* SP = savedSP[currentThread]; */ \
   { __asm \
      mov a, r1 \
      push a \
      mov a,_currentThread \
	   add a,#_savedSP \
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
       */
      bitmap = 0x0;
       
       /*
       * @@@ [2 pts]
       *     create a thread for main; be sure current thread is
       *     set to this thread ID, and restore its context,
       *     so that it starts running main().
       */
      currentThread = ThreadCreate(main);
         // while(1);
      RESTORESTATE;
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {
        /*
         * @@@ [2 pts] 
         * check to see we have not reached the max #threads.
         * if so, return -1, which is not a valid thread ID.
         */
         if (bitmap == (1 << MAXTHREADS) - 1) return -1;
        /*
         * @@@ [5 pts]
         *     otherwise, find a thread ID that is not in use,
         *     and grab it. (can check the bit mask for threads),
         *
         */
         for (createdThread = 0; createdThread < MAXTHREADS; createdThread++) {
            __asm
               push 6
               push 7
            __endasm;
            tmp = bitmap & (1 << createdThread);
            __asm
               pop 7
               pop 6
            __endasm;
            if (tmp) continue;
            break;
         }
         /*
         * @@@ [18 pts] below
         * a. update the bit mask 
             (and increment thread count, if you use a thread count, 
              but it is optional)
         */
         bitmap |= (1 << createdThread);
         /*
           b. calculate the starting stack location for new thread 
           
         */
         startStack = ((createdThread+3) << 4) | 0x0F;
         /*
           c. save the current SP in a temporary
              set SP to the starting location for the new thread
         */
         oldSP = SP;
         SP = startStack;
         /*
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
         tmp = createdThread << 3;
         __asm
                push DPL
                push DPH
                MOV A, #0x0
                push A
                push A
                push A
                push A
                push _tmp
        __endasm;

         /* savedSP[createdThread] = SP; */ 
          __asm 
            mov a, r0 
            push a 
            mov a, _createdThread 
            add a, #_savedSP 
            mov r0, a 
            mov @r0, _SP 
            pop a
            mov r0, a
         __endasm; 
         
         SP = oldSP;
         return createdThread;        
}



/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
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
               currentThread = (currentThread+1) % MAXTHREADS;
               __asm
                  push 6
                  push 7
               __endasm;
               tmp = bitmap & (1 << currentThread);
               __asm
                  pop 7
                  pop 6
               __endasm;
               if (tmp)
                  break;
                 
                 
        } while (1);
        RESTORESTATE;
}


/*
 * ThreadExit() is called by the thread's own code to terminate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
        /*
         * clear the bit for the current thread from the
         * bit mask, decrement thread count (if any),
         * and set current thread to another valid ID.
         * Q: What happens if there are no more valid threads?
         */
        RESTORESTATE;
}
