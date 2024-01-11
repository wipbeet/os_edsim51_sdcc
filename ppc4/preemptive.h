/*
 * file: preemptive.h
 *
 * this is the include file for the cooperative multithreading
 * package.  It is to be compiled by SDCC and targets the EdSim51 as
 * the target architecture.
 *
 * CS 3423 Fall 2018
 */

#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */
#define CNAME(s) _ ## s
#define LABEL(l) l ## $

typedef char ThreadID;
typedef void (*FunctionPtr)(void);
typedef char Semaphore;

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);

#define SemaphoreCreate(s, n) s = n // create a counting semaphore s that is initialized to n
#define SemaphoreWaitBody(S, label) \
    { __asm \
        LABEL(label): \
            mov a, CNAME(S) \
            jz LABEL(label) \
            jb ACC.7, LABEL(label) \
            dec  CNAME(S) \
    __endasm; \
    }

#define SemaphoreWait(s) SemaphoreWaitBody(s, __COUNTER__)      // do (busy-)wait() on semaphore s
// signal() semaphore s
#define SemaphoreSignal(s) { \
    __asm \
        INC CNAME(s) \
    __endasm; \
}

#endif // __PREEMPTIVE_H__
