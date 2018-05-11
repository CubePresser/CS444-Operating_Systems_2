# Concurrency 1: The Producer-Consumer Problem

The purpose of these concurrent programming exercises is to hone your skills in thinking in parallel. This is a very important skill, to the point where many companies view it as being as fundamental as basic algebra or being able to write code. Implementation must be in a mix of C and ASM.

For this first exercise, you will be implementing a solution to the producer-consumer problem.

In multithreaded programs there is often a division of labor between threads. In one common pattern, some threads are producers and some are consumers. Producers create items of some kind and add them to a data structure; consumers remove the items and process them.

Event-driven programs are a good example. An "event" is something that happens that requires the program to respond: the user presses a key or moves the mouse, a block of data arrives from the disk, a packet arrives from the network, a pending operation completes. Whenever an event occurs, a producer thread creates an event object and adds it to the event buffer. Concurrently, consumer threads take events out of the buffer and process them. In this case, the consumers are called "event handlers."

There are several synchronization constraints that we need to enforce to make this system work correctly:

* While an item is being added to or removed from the buffer, the buffer is in an inconsistent state. Therefore, threads must have exclusive access to the buffer.
* If a consumer thread arrives while the buffer is empty, it blocks until a producer adds a new item.
* If a producer thread has an item to put in the buffer while the buffer is full, it blocks until a consumer removes an item.

Write C code with pthreads which implements a solution which satisfies the above description. A few important details:

* The item in the buffer should be a struct with two numbers in it.
    * The first value is just a number. The consumer will print out this value as part of its consumption.
    * The second value is a random waiting period between 2 and 9 seconds, which the consumer will sleep for prior to printing the other value. This is the "work" done to consume the item.
    * Both of these values should be created using the rdrand x86 ASM instruction on systems that support it, and using the Mersenne Twister. on systems that don't support rdrand. It is your responsibility to learn how to do this, how to include it in your code, and how to condition the value such that it is within the range you want. Being able to work with x86 ASM is actually a necessary skill in this class. Hint: os-class does not support rdrand. Your laptop likely does.
* Your producer should also wait a random amount of time (in the range of 3-7 seconds) before "producing" a new item.
* Your buffer in this case can hold 32 items. It must be implicitly shared between the threads.
* Use whatever synchronization construct you feel is appropriate.
* Some helpful code:
 ~~~~
 #include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{

	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;

	char vendor[13];
	
	eax = 0x01;

	__asm__ __volatile__(
	                     "cpuid;"
	                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
	                     : "a"(eax)
	                     );
	
	if(ecx & 0x40000000){
		//use rdrand
	}
	else{
		//use mt19937
	}

	return 0;
}
 ~~~~
