#include<stdio.h>
#include<stdlib.h>
#include<mach/mach.h>
#include<mach/mach_host.h>
#include<mach/mach_init.h>
#include<mach/thread_status.h>

void my_thread_setup (thread_t t);
void my_thread_exit (void);
void my_thread_routine (int, char *);

static uintptr_t threadStack[4096];


#define EXIT_ON_MACH_ERROR(msg, retval)\
	if (kr != KERN_SUCCESS) {mach_error (msg, kr); exit((retval));}


int main (int argc, char **argv){
	thread_t 			th;
	kern_return_t 		kr;
	mach_port_name_t 	mytask, mythread;

	mytask = mach_task_self ();
	mythread = mach_thread_self ();

	//create a new thread inside our task
	kr = thread_create (mytask, &th);
	EXIT_ON_MACH_ERROR ("thread_create", kr);
	//set up the new thread's user mode execution
	my_thread_setup (th);

	//run the new thread
	kr = thread_resume (th);
	EXIT_ON_MACH_ERROR ("thread_resume", kr);

	//new thread will call exit
	//note that we still have an undiscarded reference on mythread
	thread_suspend (mythread);

	//not reached
	exit (0);
}

void my_thread_setup (thread_t th){
	kern_return_t 			kr;
	mach_msg_type_number_t 	count;
	x86_thread_state64_t    state;
	uintptr_t 				*stack = threadStack;

	//arguments to my_thread_routine -- the new function run by the new thread
	int arg1 = 16;
	char *arg2 = "hello world";

	/*stack += (PAGE_SIZE);*/

	count = x86_THREAD_STATE64_COUNT;
	kr = thread_get_state (th, 
	  						x86_THREAD_STATE64,
	   						(thread_state_t)&state, &count);

	EXIT_ON_MACH_ERROR ("thread_get_state", kr);

	state.__rip = (uintptr_t)my_thread_routine;
	*--stack = (uintptr_t)my_thread_routine;
	*--stack = (uintptr_t)my_thread_exit;
	state.__rdi = arg1;
	state.__rsi = (uintptr_t)arg2;
	state.__rsp = (uintptr_t)stack;

	kr = thread_set_state (th, x86_THREAD_STATE64, (thread_state_t)&state, x86_THREAD_STATE64_COUNT);
	EXIT_ON_MACH_ERROR ("thread_set_state", kr);
}


void my_thread_routine (int arg1, char *arg2){
	puts ("my_thread_routine()");
}

void my_thread_exit (void){
	puts ("my_thread_exit()");
	exit (0);
}

