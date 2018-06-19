/*
   mach_vm functions reference: http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/osfmk/vm/vm_user.c
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

// This is my self-resizing array structure.
typedef struct {
  mach_vm_address_t *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
  a->array = (mach_vm_address_t *)malloc(initialSize * sizeof(mach_vm_address_t));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, mach_vm_address_t element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (mach_vm_address_t *)realloc(a->array, a->size * sizeof(mach_vm_address_t));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}


// The main shit.
int main(int argc, char* argv[])
{
	// Get PID out of the argument.
	long int pid = 0;
	char *endptr = NULL;

	if( argc < 2 )
	{
		return 0;
	}

	pid = strtol( argv[ 1 ], &endptr, 10 );

	// Get task of specified PID
	kern_return_t kret;
	mach_port_t task; // type vm_map_t = mach_port_t in mach_types.defs
	kret = task_for_pid(mach_task_self(), pid, &task);

	if (kret != KERN_SUCCESS)
	{
		printf("task_for_pid() failed, error %d: %s. Forgot to run as root?\n", kret, mach_error_string(kret));
		exit(1);
	}

    // These are the arrays that will hold the memory addresses.
	Array addressArray;
	initArray(&addressArray,5);
	Array sizeArray;
	initArray(&sizeArray,5);
	Array realAddressArray;
	initArray(&realAddressArray,5);


	// Prompt user for integer to search.
	printf("Enter the value to search: ");
	int oldValue = 0; // change type: unsigned int, long, unsigned long, etc. Should be customizable!
	scanf("%d", &oldValue);


    // Initialize a bunch of memory variables.
    int occurrenceCount = 0;
	mach_vm_offset_t address = 0;
	mach_vm_size_t size;
	mach_port_t object_name;
	vm_region_basic_info_data_64_t info;
	mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
	pointer_t buffer;
	mach_msg_type_number_t bufferSize = size;
	vm_prot_t protection = info.protection;

	while (mach_vm_region(task, &address, &size, VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &count, &object_name) == KERN_SUCCESS)
	{
		if ((kret = mach_vm_read(task, (mach_vm_address_t)address, size, &buffer, &bufferSize)) == KERN_SUCCESS)
		{
			void *substring = NULL;

			if ((substring = memmem((const void *)buffer, bufferSize, &oldValue, sizeof(oldValue))) != NULL)
			{
				occurrenceCount++;
				mach_vm_address_t realAddress = (long long)substring - (long long)buffer + (long long)address;

				printf("Search result %2d: %d at 0x%llx (%s)\n", occurrenceCount, oldValue, realAddress, (protection & VM_PROT_WRITE) != 0 ? "writable" : "non-writable");
				insertArray(&addressArray,address);
				insertArray(&sizeArray,size);
				insertArray(&realAddressArray,realAddress);
			}
		}
		// else printf("mac_vm_read fails, error %d: %s\n", kret, mach_error_string(kret));
		address += size;
	}

	printf("Change value to: ");
	int newValue = 0; // change type: unsigned int, long, unsigned long, etc. Should be customizable!
	scanf("%d", &newValue);

	// printf("Enter updated value to search:");
	// int newValue = 0; // change type: unsigned int, long, unsigned long, etc. Should be customizable!
	// scanf("%d", &newValue);

	Array substringArray2; // This fill be the filtered array.
	initArray(&substringArray2, 5);

	int i;
	occurrenceCount = 0;
	for(i = 0; i < realAddressArray.used; i++)
	{
		// unpack the arrays
		mach_vm_address_t realAddress = realAddressArray.array[i];
		printf("Recovered address: 0x%0llx\n", realAddress);

		// Try to change the protections and write to the process memory.
		boolean_t set_maximum = 1;
		//task_suspend(task);
		kret = mach_vm_protect(task, realAddress, sizeof(oldValue), set_maximum, VM_PROT_READ | VM_PROT_WRITE);
		printf("Did the permission change work? %s\n", kret == KERN_SUCCESS ? "Yes" : "No");
		kret = mach_vm_write(task, realAddress, (pointer_t)&newValue, sizeof(newValue));
		printf("Write success? %s\n", kret == KERN_SUCCESS ? "Yes" : "No");
		//task_resume(task);

		if (kret != KERN_SUCCESS)
		{
			if (kret == KERN_INVALID_ADDRESS)
				printf("mach_write failed due to invalid address\n");
			else
				printf("mach_write failed due to write permission\n");
		}


		//
		// if ((substring = memmem((const void *)buffer, bufferSize, &newValue, sizeof(newValue))) != NULL)
		// {
		// 	occurrenceCount++;
		//
		// 	long realAddress = (long)substring - (long)buffer + (long)address;
		// 	printf("Search result %2d: %d at 0x%0lx (%s)\n", occurrenceCount, oldValue, realAddress, (protection & VM_PROT_WRITE) != 0 ? "writable" : "non-writable");
		// 	insertArray(&substringArray2,realAddress);
		// }
	}
	//
	// printf("Enter value to change to:");
	// int changeValue = 0; // change type: unsigned int, long, unsigned long, etc. Should be customizable!
	// scanf("%d", &changeValue);
	//
	// for(i = 0; i < substringArray2.used; i++)
	// {
	// 	mach_vm_address_t mod_address;
	// 	mod_address = substringArray2.array[i];
	//
	// 	pointer_t buffer;
	// 	mach_msg_type_number_t bufferSize = size;
	// 	vm_prot_t protection = info.protection;
	//
	// 	kret = mach_vm_write(task, mod_address, (pointer_t)&changeValue, sizeof(changeValue));
	// }

}
