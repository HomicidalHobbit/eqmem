#include "eqmem.h"

#include <iostream>
#include <mutex>

#include "bin.h"
#include "bucket.h"

std::size_t SetTags(const char* name)
{
	const std::lock_guard<std::mutex>lock(*g_global_mutex);
	g_tags->emplace_back(name);
	return g_tags->size() - 1;
}

void SetName(const char* name)
{
	t_memManager.SetName(name);
}

// Remove allocation record from thread
inline AllocatorEntry TransferFrom(void* ptr)
{
	return t_memManager.Deallocate(ptr);
}

inline void TransferTo(const AllocatorEntry& entry)
{
	t_memManager.Allocate(entry);
}

void* Allocate(std::size_t size, int tag)
{
	const std::lock_guard<std::mutex>lock(*g_global_mutex);
	return g_memManager.Allocate(size, tag, Default);
}

void* LocalAllocate(std::size_t size, int tag)
{
	return t_memManager.Allocate(size, tag, Local);	
}

void Deallocate(void* ptr)
{
	AllocatorEntry entry = t_memManager.Deallocate(ptr);

	// If we couldn't find the thread local entry, then we check the global one under a lock
	if (!entry.m_size)
	{
		const std::lock_guard<std::mutex>lock(*g_global_mutex);
		{
			entry = g_memManager.Deallocate(ptr);
		}
		if (!entry.m_size)
		{
			std::cout << "ERROR: Cannot locate" << ptr << "!!!" << std::endl;
		}
		else
		{
			free(ptr);
		}
	}
	else
	{	
		switch (entry.m_allocator)
		{
			case Default_Bucket:
			{
				reinterpret_cast<Bucket*>(entry.m_size)->Deallocate(ptr);
				break;
			}

			case Local_Bucket:
			{
				reinterpret_cast<Bucket*>(entry.m_size)->Deallocate(ptr);
				break;
			}

			default:
				free(ptr);
		}
	}
}

void CreateBucket(std::size_t size)
{
	g_memManager.AddBucket(size);
}
	
void CreateLocalBucket(std::size_t size)
{
	t_memManager.AddBucket(size);
}

std::size_t CreateBin(std::size_t size)
{
	return g_memManager.AddBin(size);
}
	
std::size_t CreateLocalBin(std::size_t size)
{
	return t_memManager.AddBin(size);
}

void SetGlobalLogging(bool enable)
{
	g_logging = enable;
}

void SetLocalLogging(bool enable)
{
	t_logging = enable;
}

