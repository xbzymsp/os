// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#include "list.h"



#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

// lab4
//List *suspended = new List();

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName,int Priority=0)
{
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;

    child = new List;
    father = NULL;
// lab4
    filename = NULL;

// lab1 
    if(thread_count < MAX_THREAD_COUNT)
    {
        for(int i=0;i<128;++i)     // set the thread_id
        {
            if(whole_thread[i].Thread_id==0) 
            {
                thread_id=i;
                whole_thread[i].Thread_id=1;
                whole_thread[i].thread=this;
                thread_count++;
                break;
            }
            continue;
        }
        set_user_id(123);     //set the user_id
    }
    
// lab1
// lab2 
    if(thread_id == 0) priority=128;
    else priority=Priority;

#ifdef USER_PROGRAM
    space = NULL;
#endif
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));

// lab6
  /*  if(!this->child->IsEmpty())
    {
printf("thread %s(tid: %d) still has child\n", name, thread_id);
        this->Sleep();
    }    
    whole_thread[thread_id].Thread_id = 0;
    whole_thread[thread_id].thread = NULL;  */
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, void *arg)
{
printf("fork %s(tid: %d  priority: %d)\n", name, thread_id, priority);
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, (int*) arg);
// lab1    
    
    if(thread_count >= MAX_THREAD_COUNT) return;  // too many threads  cannnot fork
    //thread_count++;  // when fork  the thread_count += 1

// lab1

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
//printf("ready to run\n");
    //scheduler->ReadyToRun(this);
// lab2
    scheduler->ReadyToRun2(this);
    //if(priority>currentThread->get_priority())
    //{
        //printf("qiang_zhan\n");
      //  currentThread->Yield();
    //}    
    
// lab2
 //   scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    //printf("fork\n");
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT((int) *stack == (int) STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that Scheduler::Run() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    //printf("finish\n");
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    
    threadToBeDestroyed = currentThread;
// lab1
    Thread_id[currentThread->thread_id] = 0;
    thread_count--;
// lab1
// lab4
#ifdef USER_PROGRAM
    if(currentThread->space != NULL)
    {
//printf("\nuser!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        int n = NumPhysPages;    
        for(int i=0;i<n;++i)
        {
            if(space->pPage[i])
            {
                memory->Clear(i);
                space->pPage[i] = 0;
//printf("physicalPage %d is cleared by thread %s(tid: %d)!\n",i,name,thread_id);
            }
        }
    }  
     /*   int n = space->numPages;
        for(int i=0;i<n;++i)
        {
            memory->Clear(i);
printf("physicalPage %d is cleared by thread %s(tid: %d)!\n",i,name,thread_id);
        }
    }*/
#endif
// lab6
    if(!this->child->IsEmpty())
    {
printf("thread %s(tid: %d) wants to finish but still has child\n", name, thread_id);
        //this->Sleep();
        this->Yield();
    }    
    if(father) father->child->Remove(this);

    whole_thread[thread_id].Thread_id = 0;
    whole_thread[thread_id].thread = NULL;
printf("thread %s(tid:%d) finish\n",name, thread_id);
    Sleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
    DEBUG('t', "Yielding thread \"%s\"\n", getName());
    nextThread = scheduler->FindNextToRun();
//printf("yiled\n");
//printf("thread (%s, tid:%d priority: %d) yield to thread (%s, tid: %d priority: %d)\n",
//name,thread_id, priority, nextThread->getName(),nextThread->get_thread_id(), nextThread->get_priority());
    //scheduler->ReadyToRun(this);
    //scheduler->Run(nextThread);
    if (nextThread != NULL && nextThread->get_priority()>=priority) {
//	scheduler->ReadyToRun(this);

// lab2
    scheduler->ReadyToRun2(this);
    printf("thread %s(tid:%d) yield to thread %s(tid: %d)\n",currentThread->getName(),currentThread->get_thread_id(),
    nextThread->getName(),nextThread->get_thread_id());
// lab2

	scheduler->Run(nextThread);
    }  
// lab2
    else if(nextThread != NULL) 
        scheduler->ReadyToRun2(nextThread);  
// lab2
    //printf("yield\n");
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    //printf("sleep\n");
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;

    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();	// no one to run, wait for an interrupt
        
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, void *arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE

   
    machineState[PCState] = (int*)ThreadRoot;
    machineState[StartupPCState] = (int*)InterruptEnable;
    machineState[InitialPCState] = (int*)func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int*)ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
}
#endif



/*void Thread::Suspend()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    //ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Suspending thread \"%s\"\n", getName());

    status = SUSPENDED;
    suspended->SortedInsert2(this,priority);

    NoffHeader noffH;
    OpenFile *executable = fileSystem->Open(filename);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC); 

    for(int i=0;i<space->numPages;++i)
    {
        if(space->pageTable[i].valid)
        {
            int write_back_ppn = space->pageTable[i].physicalPage;
            int write_back_vpn = space->pageTable[i].virtualPage;
            int write_back_offset = space->code_address;
            executable->WriteAt(&(machine->mainMemory[write_back_ppn*PageSize]),PageSize, 
    write_back_offset + write_back_vpn*PageSize);

        }
    }
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();
        
    scheduler->Run(nextThread); // returns when we've been signalled
} */

void
Thread::join(int tid)
{
    if(whole_thread[tid].Thread_id)
    {
//printf("thread %s(tid: %d) join thread %s(tid: %d)\n",name,thread_id,whole_thread[tid].thread->getName(),
//whole_thread[tid].thread->get_thread_id());
        whole_thread[tid].thread->father = this;
        child->Append(whole_thread[tid].thread);
    }
}