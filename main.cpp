#include <chrono>
#include <cstdio>
#include <thread>
#include "eqmem.h"

void f2()
{
	SetName("Second Thread");
	void* ptr = LocalAllocate(1024*1024 * 280);
	std::this_thread::sleep_for(std::chrono::seconds(5));
	Deallocate(ptr);
}

int main(int argc, char** argv)
{
	SetName("Main Thread");
	SetTags("Shader");
	CreateLocalBucket(64);
	CreateLocalBin(256 * 1024 * 1024);
	void* bin = LocalAllocate(1024 * 1024 * 128);
	//void* ptr = Allocate(8192);
	void* ptr2 = LocalAllocate(32768);
	void* ptr3 = LocalAllocate(64);
	void* ptr4 = LocalAllocate(64 * 1024 * 1024);
	void* ptr5 = LocalAllocate(64);
	
	std::thread t2(f2);
	t2.join();
	Deallocate(ptr5);
	//Deallocate(ptr);
	Deallocate(ptr2);
	Deallocate(ptr3);
	Deallocate(ptr4);
	Deallocate(bin);
	return 0;
}