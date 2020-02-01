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

	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		m->SetName(name);
	}
}

void* Allocate(std::size_t size, int tag)
{
	const std::lock_guard<std::mutex>lock(*g_global_mutex);
	return g_memManager.Allocate(size, tag, Default);
}

void* LocalAllocate(std::size_t size, int tag)
{
	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		return m->Allocate(size, tag, Local);	
	}
	else
	{
		return nullptr;
	}
}

void Deallocate(void* ptr)
{
	std::cout << "dealloc with: " << t_threadMemManager << std::endl;
	if (!t_threadMemManager)
	{
		free(ptr);
		return;
	}

	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		// Check for any waiting transients
		if (g_transients_waiting)
		{
			const std::lock_guard<std::mutex>lock(*g_transient_mutex);
			for (auto it = g_transients.begin(); it != g_transients.end(); )
			{
				AllocatorEntry entry = m->Deallocate(*it);
				if (entry.m_size)
				{
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
	}

	AllocatorEntry entry = m->Deallocate(ptr);

	// If we couldn't find the thread local entry, then we check the global one under a lock
	if (!entry.m_size)
	{
		const std::lock_guard<std::mutex>lock(*g_global_mutex);
		{
			entry = g_memManager.Deallocate(ptr);
		}
		if (!entry.m_size)
		{	
			// Mark as transient for other threads
			const std::lock_guard<std::mutex>lock(*g_transient_mutex);
			g_transients.push_back(ptr);
			g_transients_waiting = true;
		}
		else
		{
			// Global allocator frees
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
	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		m->AddBucket(size);
	}
}

std::size_t CreateBin(std::size_t size)
{
	return g_memManager.AddBin(size);
}
	
std::size_t CreateLocalBin(std::size_t size)
{
	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		return m->AddBin(size);
	}
	else
	{
		return 0;
	}
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
	MemManager* m = t_threadMemManager->GetMemManager();
	if (m)
	{
		m->DisplayAllocations();
	}
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

