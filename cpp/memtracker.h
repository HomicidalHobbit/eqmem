#pragma once

#include <vector>

enum Allocator
{
	Default,
	Local,
	Default_Bucket,
	Local_Bucket
};

struct AllocatorEntry
{
	std::size_t m_size;
	int m_tag;
	Allocator m_allocator;
};

class MemTracker
{

public:
	MemTracker();
	~MemTracker();
	void StoreAllocatorEntry(void * ptr, std::size_t size, int tag, Allocator allocator);
	std::size_t FindAllocatorEntry(void* ptr);
	const AllocatorEntry& GetAllocatorEntry(std::size_t index);
	void EraseAllocatorEntry(std::size_t index);
	const bool Empty();

	std::vector<std::vector<void*>> m_ptr;
	std::vector<std::vector<AllocatorEntry>> m_allocatorEntry;
};