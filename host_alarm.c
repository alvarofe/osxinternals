#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>


#define OUT_ON_MACH_ERROR(msg, retval)\
	if (kr != KERN_SUCCESS) {mach_error (msg, kr); goto out;} 


//structure of the IPC message that will receive from the clock

typedef struct msg_format_recv_s {
	mach_msg_header_t	header;
	int					data;
	mach_msg_trailer_t	trailer;
} msg_format_recv_t;


int main(){
	kern_return_t		kr;
	clock_serv_t		clk_system;
	mach_timespec_t 	alarm_time;
	clock_reply_t		alarm_port;
	struct timeval		t1, t2;
	msg_format_recv_t	message;
	mach_port_t			mytask;

	mytask = mach_task_self();

	kr = host_get_clock_service (mach_host_self(), SYSTEM_CLOCK,
							(clock_serv_t *)&clk_system);

	OUT_ON_MACH_ERROR ("host_get_clock_service", kr);

	//let us set the alarm to ring after 2.5 seconds
	alarm_time.tv_sec = 2;
	alarm_time.tv_nsec = 50000000;

	//Allocate a port with receive right

	kr = mach_port_allocate (
			mytask,
			MACH_PORT_RIGHT_RECEIVE,
			&alarm_port);

	OUT_ON_MACH_ERROR ("mach_port_allocate", kr);

	gettimeofday (&t1, NULL);

	//set the alarm

	kr = clock_alarm (clk_system, 
			TIME_RELATIVE,
			alarm_time,
			alarm_port);

	OUT_ON_MACH_ERROR ("clock_alarm", kr);

	printf ("Current time %ld s + %d us\n"
			"Setting alarm to ring after %d s + %d ns",
			t1.tv_sec, t1.tv_usec, alarm_time.tv_sec, alarm_time.tv_nsec);

	//will block

	kr = mach_msg (&(message.header),		//the message buffer
			MACH_RCV_MSG,					//message options bits
			0,								//send size
			message.header.msgh_size,		//receive limit
			alarm_port,						//receive right
			MACH_MSG_TIMEOUT_NONE,			//no timeout
			MACH_PORT_NULL);				//no timeout notification port

	gettimeofday (&t2,NULL);
	OUT_ON_MACH_ERROR ("mach_msg", kr);

	if (t2.tv_usec < t1.tv_usec){
		t1.tv_sec += 1;
		t1.tv_usec -= 1000000;
	}

	printf ("\nCurrent time %ld s + %d us\n", t2.tv_sec, t2.tv_usec);
	printf ("Alarm rang after %ld s + %d us\n", (t2.tv_sec - t1.tv_sec),
			(t2.tv_usec - t1.tv_usec));

out:
	
	mach_port_deallocate (mytask, clk_system);
	mach_port_deallocate (mytask, alarm_port);

	exit (0);
}
