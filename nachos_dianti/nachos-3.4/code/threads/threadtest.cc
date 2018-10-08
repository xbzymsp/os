// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

#include "synch.h"

extern const int MAX_THREAD_COUNT;
extern int thread_count;
extern struct Whole_thread whole_thread[128];

// lab3
#define BUFFER_LENGTH 4

Lock *my_lock = new Lock("lock");
Condition *my_condition = new Condition("condition");
Condition *slots = new Condition("slots");
Condition *items = new Condition("items");
Semaphore *mutex = new Semaphore("mutex",1);
//Semaphore *slots = new Semaphore("slots",3);
//Semaphore *items = new Semaphore("items",0);

bool s=false;
bool buffer[BUFFER_LENGTH]={0};
int item_num=0;

bool buffer_all_ture()
{
    bool ans = true;
    for(int i=0;i<BUFFER_LENGTH;++i)
        ans = ans && buffer[i];
    return ans;
}

void insert()
{
    for(int i=0;i<BUFFER_LENGTH;++i)
    {
        if(!buffer[i]) 
        {
            buffer[i]=true;
            item_num++;
            return;
        }
    }
}

void remove()
{
    for(int i=0;i<BUFFER_LENGTH;++i)
    {
        if(buffer[i])
        {
            buffer[i]=false;
            item_num--;
            return;
        }
    }
}


// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread4();

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 16; num++) {
	printf("thread %d looped %d times priority: %d\n",currentThread->get_thread_id(),num,currentThread->get_priority());
        //currentThread->Yield();
        //printf("test\n");
        interrupt->SetLevel(IntOn);
        interrupt->SetLevel(IntOff);
    }

   // currentThread->Finish();
}

void
SimpleThread2()    // lab2
{
    for(int i=0;i<5;++i)
    {
        printf("No.%d thread %d priority: %d\n",i,currentThread->get_thread_id(),currentThread->get_priority());
        interrupt->SetLevel(IntOn);
        interrupt->SetLevel(IntOff);
    }
}

void
SimpleThread5()   
{
    printf("thread %d is running! it's priority is %d\n",currentThread->get_thread_id(),currentThread->get_priority());
    Thread *t5 = new Thread("forked thread",50);
    t5->Fork(SimpleThread4, t5->get_thread_id());
    printf("thread %d is running! it's priority is %d\n",currentThread->get_thread_id(),currentThread->get_priority());
}

void
SimpleThread6()   
{
    printf("thread %d is running! it's priority is %d\n",currentThread->get_thread_id(),currentThread->get_priority());
}

void
SimpleThread3()   // lab3
{
    my_lock->Acquire();
    printf("thread %d wants to insert\n",currentThread->get_thread_id());
    while(item_num == BUFFER_LENGTH)
    {
        printf("no slot in buffer so thread %d Sleep()\n",currentThread->get_thread_id());
        slots->Wait(my_lock);
    }
    printf("thread %d successfully inserted\n",currentThread->get_thread_id());
    insert();
    items->Signal(my_lock);
    my_lock->Release();
}

void
SimpleThread4()  // lab3
{   
    my_lock->Acquire();
    printf("thread %d wants to remove\n",currentThread->get_thread_id());
    while(item_num == 0)
    {
        printf("no item in buffer so thread %d Sleep()\n",currentThread->get_thread_id());
        items->Wait(my_lock);
    }
    printf("thread %d successfully romved\n",currentThread->get_thread_id());
    remove();
    slots->Signal(my_lock);
    my_lock->Release();
}

void
SimpleThread7()
{   
    my_lock->Acquire();
    for(int i=0;i<4;++i)
    {
        printf("thread %d looped %d times\n",currentThread->get_thread_id(),i);
        if(i == 2)
        {
            buffer[currentThread->get_thread_id()-1] = true;
            if(!buffer_all_ture()) 
            {
                printf("still some threads have not arrived so thread %d Sleep()\n",currentThread->get_thread_id());
                my_condition->Wait(my_lock);
            }
            if(!my_condition->no_wait())
            {
                printf("all the threads have arrived so Broadcast()\n");
                my_condition->Broadcast(my_lock);
            }
        }
    }
    my_lock->Release();
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    

    Thread *t1 = new Thread("forked thread",130);
   /* Thread *t2 = new Thread("forked thread",40);
    Thread *t3 = new Thread("forked thread",40);
    Thread *t4 = new Thread("forked thread",40); */

    t1->Fork(SimpleThread, t1->get_thread_id());
  /*  t2->Fork(SimpleThread, t2->get_thread_id());
    t3->Fork(SimpleThread, t3->get_thread_id());
    t4->Fork(SimpleThread, t4->get_thread_id());  */
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
//----------------------------------------------------------------------

void
ThreadTest2()  // lab2
{
    DEBUG('t', "Entering ThreadTest2");

    Thread *t1 = new Thread("forked thread",20);
    Thread *t2 = new Thread("forked thread",10);
    Thread *t3 = new Thread("forked thread",10);
    Thread *t4 = new Thread("forked thread",129);

    t1->Fork(SimpleThread2, t1->get_thread_id());
    t2->Fork(SimpleThread2, t2->get_thread_id());
    t3->Fork(SimpleThread2, t3->get_thread_id());
    t4->Fork(SimpleThread2, t4->get_thread_id());
  
    printf("thread %d priority: %d\n",currentThread->get_thread_id(),currentThread->get_priority()); 
}

void
ThreadTest3()  // lab3
{
    DEBUG('t', "Entering ThreadTest2");

    int pr = 127;
    for(int i=0;i<BUFFER_LENGTH+1;++i)
    {
        Thread *t = new Thread("insert",pr);
        t->Fork(SimpleThread3,t->get_thread_id());
    }
    for(int i=0;i<BUFFER_LENGTH+1;++i)
    {
        Thread *t = new Thread("remove",pr);
        t->Fork(SimpleThread4,t->get_thread_id());
    }
    Thread *t = new Thread("insert",pr);
    t->Fork(SimpleThread3,t->get_thread_id());
}

void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest2");

    int pr = 0;
    for(int i=0;i<BUFFER_LENGTH;++i)
    {
        Thread *t = new Thread("buffer_all_true",pr--);
        t->Fork(SimpleThread7,t->get_thread_id());
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    default:
	printf("No test specified.\n");
	break;
    }
}

