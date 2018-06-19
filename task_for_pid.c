#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>

int main( int argc, char** argv )
{
	kern_return_t kern_return = 0;
	mach_port_t task = 0;
	long int pid = 0;
	char *endptr = NULL;

	if( argc < 2 )
	{
		return 0;
	}

	pid = strtol( argv[ 1 ], &endptr, 10 );
	kern_return = task_for_pid( mach_task_self(), pid, &task );

	if( kern_return != KERN_SUCCESS )
	{
		printf( "task_for_pid failed: %s\n", mach_error_string( kern_return ) );
		return 0;
	}

	printf( "%u\n", task );

	return 0;
}
