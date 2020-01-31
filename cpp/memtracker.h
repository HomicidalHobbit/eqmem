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
	int FindAllocatorEntry(void* ptr);
	const AllocatorEntry& GetAllocatorEntry(int index);
	void EraseAllocatorEntry(int index);
	const bool Empty() const { return m_ptr.empty(); }

private:
	std::vector<void*> m_ptr;
	std::vector<AllocatorEntry> m_allocatorEntry;
};