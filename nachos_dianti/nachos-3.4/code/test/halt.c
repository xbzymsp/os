/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int tmp;
int read_num;
char *buffer;
void print(int a)
{
    Exit(12345);
}
void func()
{
    Create("msp");
    Exit(1);
}
void join_test(int id)
{
    Join(id);
}
int
main()
{
    //Create("test_file1");
    //tmp = Open("test_file1");
    //read_num = Read(buffer, 3, tmp);
    //Write("abcdefg", 7, tmp);
    //Close(tmp);
    //Read(result, 7, tmp);
    //Exec("matmult");
    Fork(func);
    Join(1);
    Exit(2);
    //Yield();
    //Exit(123);
    //Yield();
    //Halt();
    /* not reached */
}
