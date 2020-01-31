#include "eqmem.h"

#include <atomic>
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
	// Check for any waiting transients
	if (g_transients_waiting)
	{
		const std::lock_guard<std::mutex>lock(*g_transient_mutex);
		for (auto it = g_transients.begin(); it != g_transients.end(); )
		{
			AllocatorEntry entry = t_memManager.Deallocate(*it);
			if (entry.m_size)
			{
				std::cout << "deallocated transient" << std::endl;
				it = g_transients.erase(it);
			}
			else
			{
				++it;
			}
		}
		if (g_transients.empty())
		{
			g_transients_waiting = false;
		}
	}

	AllocatorEntry entry = t_memManager.Deallocate(ptr);

	// If we couldn't find the thread local entry, then we check the global one under a lock
	if (!entry.m_size)
	{
		t_memManager.DisplayThread();
		//std::cout << "could not find " << ptr;
		const std::lock_guard<std::mutex>lock(*g_global_mutex);
		{
			entry = g_memManager.Deallocate(ptr);
		}
		if (!entry.m_size)
		{
			//std::cout << " ERROR: Cannot locate " << ptr << " in Global Manager!!!" << std::endl;
			
			// Mark as transient for other threads
			const std::lock_guard<std::mutex>lock(*g_transient_mutex);
			g_transients.push_back(ptr);
			g_transients_waiting = true;
		}
		else
		{
			//std::cout << ptr << " was found in Global Manager" << std::endl;
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

void DisplayGlobalAllocations()
{
	g_memManager.DisplayAllocations();
}

void DisplayLocalAllocations()
{
	t_memManager.DisplayAllocations();
}

void* Reallocate(void* ptr, std::size_t size)
{
//	std::cout << "[Reallocating] ";
	Deallocate(ptr);
//	std::cout << "[Reallocating] ";
	return LocalAllocate(size, 0);
}

void* Malloc(std::size_t size)
{
	std::cout << "Malloc of " << size << " at: ";
	void* ptr = malloc(size);
	std::cout << ptr << std::endl;
	return ptr;
}

void Free(void* ptr)
{
	std::cout << "Free: " << ptr << std::endl;
	free(ptr);
}

void* Realloc(void* ptr, std::size_t size)
{
	std::cout << "Realloc: " << ptr << " to size " << size;
	void* new_ptr = realloc(ptr, size);
	std::cout << " now at: " << new_ptr << std::endl;
	return new_ptr;
}

