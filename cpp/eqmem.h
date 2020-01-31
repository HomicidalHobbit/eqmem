#pragma once

#include <cstddef>
#include "memmanager.h"

extern "C"
{
	void* Allocate(std::size_t size, int tag = 0);
	void* LocalAllocate(std::size_t size, int tag = 0);
	void Deallocate(void* ptr);
	void* Reallocate(void* ptr, std::size_t size);
	std::size_t CreateBin(std::size_t size);
	std::size_t CreateLocalBin(std::size_t size);
	void CreateBucket(std::size_t size);
	void CreateLocalBucket(std::size_t size);
	void SetName(const char* name);
	std::size_t SetTags(const char* name);
	AllocatorEntry TransferFrom(void* ptr);
	void TransferTo(const AllocatorEntry& entry);
	void SetGlobalLogging(bool enable);
	void SetLocalLogging(bool enable);
	void DisplayGlobalAllocations();
	void DisplayLocalAllocations();

	void* Malloc(std::size_t size);
	void Free(void* ptr);
	void* Realloc(void* ptr, std::size_t size);
}


