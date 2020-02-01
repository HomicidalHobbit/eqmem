#include "memtracker.h"
#include <iostream>


MemTracker::MemTracker()
{
	m_ptr.reserve(16);
	m_allocatorEntry.reserve(16);
	for (int i = 0; i < 16; ++i)
	{
		m_ptr.emplace_back(std::vector<void*>());
		m_allocatorEntry.emplace_back(std::vector<AllocatorEntry>());
	}
}

MemTracker::~MemTracker()
{
	std::cout << "EXIT MemTracker!!!" << std::endl;
}

const bool MemTracker::Empty()
{
	for (std::size_t b = 0; b < 16; ++b)
	{
		if (!m_ptr[b].empty())
		{
			return false;
		}	
	}
	return true;
}

void MemTracker::StoreAllocatorEntry(void *ptr, std::size_t size, int tag, Allocator allocator)
{
	// Shift address down to low nybble and mask off to find correct bucket
	std::size_t index = (reinterpret_cast<std::size_t>(ptr) >> 20) & 15;
	m_ptr[index].push_back(ptr);
	m_allocatorEntry[index].emplace_back(AllocatorEntry { size, tag, allocator} );
}

std::size_t MemTracker::FindAllocatorEntry(void* ptr)
{
	std::size_t address = reinterpret_cast<std::size_t>(ptr);
	std::size_t index = ((address & 0xf00000) << 12);
	std::size_t bucket = (address >> 20) & 15;
	for (auto it = m_ptr[bucket].begin(); it != m_ptr[bucket].end(); ++it, ++index)
	{
		if (*it == ptr)
		{
			return index + 1;
		}
	}
	return 0;
}

const AllocatorEntry& MemTracker::GetAllocatorEntry(std::size_t index)
{
	return m_allocatorEntry[index >> 32][(index & 0xFFFFFFFF) - 1];
}

void MemTracker::EraseAllocatorEntry(std::size_t index)
{
//	std::cout << "index: " << index << std::endl;
	std::size_t bucket = index >> 32;
	std::size_t i = (index & 0xFFFFFFFF) - 1;
	m_ptr[bucket].erase(m_ptr[bucket].begin() + i);
	m_allocatorEntry[bucket].erase(m_allocatorEntry[bucket].begin() + i);
}