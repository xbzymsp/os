// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//lab4
void multithreading_test(int id)
{
    printf("thread %s(tid: %d) is running!\n",currentThread->getName(),id);
   // Suspend();
    machine->Run();
}

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    //char *filename2 = "../test/halt";
    //OpenFile *executable2 = fileSystem->Open(filename2);
    AddrSpace *space;
    //AddrSpace *space2;

    if ((executable == NULL) ) {
	printf("Unable to open file %s or %s\n", filename);
	return;
    }  
    space = new AddrSpace(executable);    
    currentThread->space = space;
    currentThread->filename = filename;
    /*space2 = new AddrSpace(executable2);
    space2->InitRegisters();		// set the initial register values
    space2->RestoreState();		// load page table register
    Thread *thread2 = new Thread("halt",1);
    thread2->space = space2;
    thread2->filename = filename2;
    thread2->Fork(multithreading_test,thread2->get_thread_id());  */

    delete executable;			// close file
    //delete executable2;

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    //printf("run user program\n");

    //Thread *tmp = scheduler->FindNextToRun();
    //if(tmp) scheduler->Run(tmp);

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
