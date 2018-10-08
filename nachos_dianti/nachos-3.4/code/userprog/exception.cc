// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void PageFaultHandler();
void TlbMissHandler();
extern void pagefault_lru(int addr, char *filename);

void fork_func(int pc);
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
// lab4  memory recover
    else if((which == SyscallException) && (type == SC_Exit)) 
    {
//printf("exit\n");
        //printf("\nsyscall!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11\n");
        int a = machine->ReadRegister(4);
printf("Exit_status: %d\n",a);
        currentThread->Finish();
        //interrupt->Halt();
    }
    else if((which == SyscallException) && (type == SC_Create))
    {
        int arg = machine->ReadRegister(4);
        int name;
        int length=0;
        do
        {
            machine->ReadMem(arg++,1,&name);
            length++;
        }while(name!=0);
        arg -= length;
//printf("length: %d\n",length);
        char filename[100];
        for(int i=0;i<length;++i)
        {
            machine->ReadMem(arg+i, 1, &name);
            filename[i] = (char)name;
        }
//printf("file name: %s\n",filename);
        fileSystem->Create(filename, 1024);
        machine->advance_pc();
        //interrupt->Halt();
    }
    else if((which == SyscallException) && (type == SC_Open))
    {
        int arg = machine->ReadRegister(4);
        int name;
        int length = 0;
        do
        {
            machine->ReadMem(arg++,1,&name);
            length++;
        }while(name!=0);
        arg -= length;
//printf("length: %d\n",length);
        char filename[100];
        for(int i=0;i<length;++i)
        {
            machine->ReadMem(arg+i, 1, &name);
            filename[i] = (char)name;
        }
        OpenFile *o = fileSystem->Open(filename);
        int tmp = o->file;
//printf("open file name: %s  id: %d\n",filename,tmp);
        machine->WriteRegister(2, o->file);
        machine->advance_pc();
        //interrupt->Halt();        
    }
    else if((which == SyscallException) && (type == SC_Close))
    {
        int arg = machine->ReadRegister(4);
        Close(arg);
printf("close file id:%d\n",arg);
        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Write))
    {
        int base = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int file = machine->ReadRegister(6);
        int value;
        int length =0;
        do
        {
            machine->ReadMem(base++, 1, &value);
            length++;
        }while(value!=0);
        base -= length;
        char content[1000];
//printf("length: %d\n",length);
        int i = 0;
        for(i=0;i<length-1;++i)
        {
//printf("i: %d\n",i);
            machine->ReadMem(base+i, 1, &value);
            content[i] = (char)value;
//printf("value: %c\n",content[i]);
        }
        content[i] = '\0';
printf("write file id: %d    write content: %s\n",file,content);
        OpenFile * o = new OpenFile(file);
        o->Write(content,size);
        delete o;

        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Read))
    {
        int base = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int file = machine->ReadRegister(6);

        OpenFile *o = new OpenFile(file);
        char tmp[1000];
        int read_num = 0;
        read_num = o->Read(tmp, size);
        int i = 0;
        for(i=0;i<size;++i)
        {
            machine->WriteMem(base, 1, tmp[i]);
        }        
        tmp[i] = '\0';
printf("read_num: %d  content: %s\n",read_num,tmp);
        machine->WriteRegister(2,read_num);
        delete o;
        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Exec))
    {      
        int base = machine->ReadRegister(4);
        int value;
        int length = 0;
        do
        {
            machine->ReadMem(base++, 1, &value);
            length++;
        }while(value != 0);
        base -= length;
        char filename[100];
        for(int i=0;i<length;++i)
        {
            machine->ReadMem(base+i, 1, &value);
            filename[i] = (char)value;
        }
        OpenFile *o = fileSystem->Open(filename);
        AddrSpace *space;
        if(o == NULL)
        {
printf("can not find file %s\n",filename);
        machine->advance_pc();
        return;
        }
        space = new AddrSpace(o);
        currentThread->space = space;
        currentThread->filename = filename;
        delete o;
        space->InitRegisters();
        space->RestoreState();
printf("now run user program: %s\n",filename);
        machine->Run();

        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Fork))
    {      
        int pc = machine->ReadRegister(4);
        AddrSpace *space = currentThread->space;
        Thread *t = new Thread("fork_thread");
        t->space = space;
//printf("fork thread id: %d\n",t->get_thread_id());
        t->Fork(fork_func, pc);
        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Yield))
    {      
//printf("syscall yield\n");
        //machine->advance_pc();
        currentThread->Yield();
        machine->advance_pc();
    }
    else if((which == SyscallException) && (type == SC_Join))
    {      
        int arg = machine->ReadRegister(4);
//printf("arg: %d\n",arg);
        currentThread->join(arg);
        if(whole_thread[arg].Thread_id == 1 && whole_thread[arg].thread->father == currentThread)
        {
printf("thread %d wait thread %d\n",currentThread->get_thread_id(),arg);
            currentThread->Yield();
        }
        machine->advance_pc();
    }
    else if(which == PageFaultException)
    {
        if(machine->tlb == NULL) PageFaultHandler();
        else TlbMissHandler();
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

// lab4
void
PageFaultHandler()
{
    //printf("Page Fault!\n");
    //ASSERT(FALSE);
    int vAddr,vpn,offset;
    vAddr = machine->ReadRegister(BadVAddrReg);
//printf("\n pagefaultHandler!\n\n");
    pagefault_lru(vAddr,currentThread->filename);
}

void
TlbMissHandler()
{
    int virtAddr,vpn,offset;
    virtAddr = machine->ReadRegister(BadVAddrReg);
    //machine->fifo(virtAddr);
    machine->lru(virtAddr);
}

void
fork_func(int pc)
{
    machine->WriteRegister(PCReg,pc);
    machine->WriteRegister(NextPCReg,pc+4);
    machine->Run();
}











/*  initial handler
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
*/
