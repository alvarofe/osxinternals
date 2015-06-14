#include <dlfcn.h>
#include <xpc/xpc.h>



void hexdump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

mach_msg_return_t (*orig_mach_msg)(mach_msg_header_t *, 
	mach_msg_option_t, mach_msg_size_t, 
	mach_msg_size_t, 
	mach_port_t, 
	mach_msg_timeout_t, 
	mach_port_t);

mach_msg_return_t mach_msg(mach_msg_header_t *msg, 
	mach_msg_option_t option, 
	mach_msg_size_t send_size, 
	mach_msg_size_t rcv_size, 
	mach_port_t rcv_name, 
	mach_msg_timeout_t timeout, 
	mach_port_t notify){

	if(!orig_mach_msg){
		orig_mach_msg = dlsym(RTLD_NEXT, "mach_msg");
	}
	
	// The mach message's local_port is how a response will be sent  
	// back. If this is set, we assume there will be a response.
	bool response = msg->msgh_local_port > 0;

	// Request
	hexdump("\nrequest", msg, send_size);
	mach_msg_return_t ret = orig_mach_msg(msg, 
		option, 
		send_size, 
		rcv_size, 
		rcv_name, 
		timeout, 
		notify);
	
	// Response
	if(response){
		hexdump("response", msg, rcv_size);
	}

	return(ret);
}
