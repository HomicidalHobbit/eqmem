#include "memtracker.h"

#include <iostream>


MemTracker::MemTracker()
{
	std::cout << "MemTracker Start" << std::endl;
}

MemTracker::~MemTracker()
{
	std::cout << "MemTracker End" << std::endl;
}

void MemTracker::StoreAllocatorEntry(void *ptr, std::size_t size, int tag, Allocator allocator)
{
	m_ptr.push_back(ptr);
	m_allocatorEntry.emplace_back(AllocatorEntry { size, tag, allocator });
}

int MemTracker::FindAllocatorEntry(void* ptr)
{
	int index = 1;
	for (auto it = m_ptr.begin(); it != m_ptr.end(); ++it, ++index)
	{
		if (*it == ptr)
		{
			return index;
		}
	}
	return 0;
}

const AllocatorEntry& MemTracker::GetAllocatorEntry(int index)
{
	return m_allocatorEntry[index - 1];
}

void MemTracker::EraseAllocatorEntry(int index)
{
	std::cout << "index: " << index << std::endl;
	int i = index - 1;
	m_ptr.erase(m_ptr.begin() + i);
	m_allocatorEntry.erase(m_allocatorEntry.begin() + i);
}