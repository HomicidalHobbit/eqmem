#include "memmanager.h"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "bin.h"
#include "bucket.h"

#define MAX_BUCKET_ENTRY_SIZE 256

//thread_local MemManager t_memManager;
thread_local bool t_logging(true);
thread_local std::unique_ptr<ThreadMemManager> t_threadMemManager(new ThreadMemManager);

bool g_logging(true);
bool g_leakWarning(true);
MemManager g_memManager(THREADSAFE);
std::atomic<std::size_t> g_allocated(0);
std::atomic<std::size_t> g_managerCount(0);
std::atomic<bool> g_initialised(false);
std::vector<std::string> g_allocators;
std::mutex* g_global_mutex = nullptr;
std::vector<std::string>* g_tags = nullptr;
std::atomic<bool> g_transients_waiting(false);
std::mutex* g_transient_mutex = nullptr;
std::vector<void*> g_transients;

ThreadMemManager::ThreadMemManager()
{
	m_memManager = new MemManager();
}

ThreadMemManager::~ThreadMemManager()
{
	std::cout << "Deleting ThreadManager" <<std::endl;
	delete m_memManager;
}

MemManager::MemManager(bool threadsafe)
: m_memTracker(new MemTracker)
, m_isGlobal(threadsafe)
{	
	m_allocated = 0;
	m_used = false;
	m_tid = std::this_thread::get_id();
	std::stringstream ss;
	ss << m_tid;
	m_id = (std::stoul(ss.str(), nullptr, 16));
	if (!g_initialised)
	{
		g_initialised = true;
		std::cout << "Starting EQMem!\n";
		g_global_mutex = new std::mutex;
		g_transient_mutex = new std::mutex;
		const std::lock_guard<std::mutex>lock(*g_global_mutex);
		g_tags = new std::vector<std::string>;
		g_tags->emplace_back("Default");
		g_allocators.emplace_back("Global");
		g_allocators.emplace_back("Local");
	}
	if (m_isGlobal && g_logging)
	{
			std::cout << "Created Global MemManager!\n";
	}
	else
	{
		if (t_logging)
		{
			std::cout << "Created MemManager for thread: 0x" << 
			std::hex << m_id << std::dec << std::endl;
		}
	}
	++g_managerCount;	
}

MemManager::~MemManager()
{
	std::cout << "EXIT MemManager!!!" << std::endl;
	if (m_used && g_leakWarning)
	{
		if (!m_memTracker->Empty())
		{
			if (m_isGlobal)
			{
				std::cout << "ERROR: Global Leaks:" << std::endl;
			}
			else
			{
				std::cout << "ERROR: Leaks in thread: ";
				DisplayThread();
				std::cout << std::endl;
			}
			
			/*
			std::size_t index = 0;
			for (const auto& allocatorEntry : m_memTracker.m_allocatorEntry)
			{
				std::size_t size;
				switch (allocatorEntry.m_allocator)
				{
					case Default_Bucket:
					{
						size = reinterpret_cast<Bucket*>(allocatorEntry.m_size)->GetSlotSize();
						break;
					}

					case Local_Bucket:
					{
						size = reinterpret_cast<Bucket*>(allocatorEntry.m_size)->GetSlotSize();
						break;
					}

					default:
						size = allocatorEntry.m_size;
				}
				std::cout << "'" << g_tags->at(allocatorEntry.m_tag) <<
				"' " << m_memTracker.m_ptr[index] << " of size ";
				ReportSize(size);
				std::cout << std::endl;
				free(m_memTracker.m_ptr[index++]);
			}
			*/
		}
	}
	if (!--g_managerCount)
	{
		delete g_transient_mutex;
		delete g_global_mutex;
		delete g_tags;	
		std::cout << "Shutting Down EQMem!" << std::endl;		
	}
	if (m_memTracker != nullptr)
	{
		delete m_memTracker;
		m_memTracker = nullptr; 
	}
}

void MemManager::DisplayAllocations()
{
	if (m_memTracker)
	{
		std::cout << "Current Allocations for ";
		DisplayThread();
		std::cout << std::endl;
		std::size_t count = 0;
		for (std::size_t b = 0; b < 16; ++b)
		{
			std::size_t index = 0;
			for (const auto& ptr : m_memTracker->m_ptr[b])
			{
				std::cout << count++ << "\t" << ptr << " size: ";
				ReportSize(m_memTracker->m_allocatorEntry[b][index++].m_size);
				std::cout << std::endl;		
			}
		}
	}
}

void MemManager::DisplayThread()
{
	if (m_name.empty())
	{
		std::cout << m_tid;
	}
	else
	{
		std::cout << "'" << m_name << "'";
	}
	std::cout << " ";
}

void MemManager::DisplayTime()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::cout << std::put_time(std::localtime(&t), "%T") << " ";
}

void MemManager::ReportSize(std::size_t size)
{
	if (size < 1024)
	{
		std::cout << size << " bytes ";
	}
	else if (size < 1024 * 1024)
	{
		if (size % 1024)
		{
			std::cout << std::fixed << std::setprecision(2);
			std::cout << (float)size / 1024.0f << "K ";
		}
		else
		{
			std::cout << (size >> 10) << "K ";
		}
	}
	else if (size < 1024 * 1024 * 1024)
	{
		if (size % 1024 * 1024)
		{
			std::cout << std::fixed << std::setprecision(2);
			std::cout << (float)size / 1024.0f / 1024.0f << " MB ";
		}
		else
		{
			std::cout << (size >> 20) << " MB ";
		}
	}
	else
	{
		if (size % 1024 * 1024 * 1024)
		{
			std::cout << std::fixed << std::setprecision(2);
			std::cout << (float)size / 1024.0f / 1024.0f / 1024.0f << " GB ";
		}
		else
		{
			std::cout << (size >> 30) << " GB ";	
		}
	}
}

Bucket* MemManager::FindBucket(std::size_t size)
{
	Bucket* bucket_ptr = nullptr;
	if (m_isGlobal)
	{
		m_bucket_mutex.lock();
	}
	for (auto& bucket : m_buckets)
	{
		if (bucket->GetSlotSize() == size && bucket->GetFree())
		{
			bucket_ptr = bucket.get();
			break;
		}
	}
	if (m_isGlobal)
	{
		m_bucket_mutex.unlock();
	}
	return bucket_ptr;
}

void* MemManager::Allocate(std::size_t size, int tag, Allocator allocator)
{
	void* ptr = nullptr;
	/*
	if (size <= MAX_BUCKET_ENTRY_SIZE)
	{
		if (Bucket* bucket = FindBucket(size))
		{
			ptr = bucket->Allocate();
			if (ptr)
			{
				m_map[ptr] = AllocatorEntry{ reinterpret_cast<std::size_t>(bucket),
				tag, m_isGlobal ? Default_Bucket : Local_Bucket};
			}
		}
	}
	*/
	if (!ptr)
	{
		ptr = malloc(size);	
		if (m_memTracker)
		{
			m_memTracker->StoreAllocatorEntry(ptr, size, tag, allocator);
		}
		g_allocated += size;
		m_allocated += size;
	}
		
	if (m_isGlobal && g_logging)
	{
		DisplayTime();
		std::cout << "alloc ";
		ReportSize(size);
		std::cout << "to: " << 
		ptr << " - total memory: ";
		ReportSize(g_allocated); 
		std::cout << std::endl;	
	}
	else
	{	
		if (t_logging)
		{
			DisplayTime();
			DisplayThread();
			std::cout << "alloc "; 
			ReportSize(size);
			std::cout << "to: " << 
			ptr << " - thread memory: "; 
			ReportSize(m_allocated);
			std::cout << ", total memory: "; 
			ReportSize(g_allocated);
			std::cout << std::endl;	
		}
	}
	m_used = true;
	return ptr;
}

AllocatorEntry MemManager::Deallocate(void* ptr)
{
	AllocatorEntry entry;
	if (!m_memTracker)
	{
		entry.m_size = 0;
		return entry;
	}
	std::size_t index = m_memTracker->FindAllocatorEntry(ptr);

	if (!index)
	{
		entry.m_size = 0;
	}
	else 
	{
		std::size_t size;
		entry = m_memTracker->GetAllocatorEntry(index);
//		entry = search->second;
		switch (entry.m_allocator)
		{
			case Default_Bucket:
			{
				size = reinterpret_cast<Bucket*>(entry.m_size)->GetSlotSize();
				break;
			}

			case Local_Bucket:
			{
				size = reinterpret_cast<Bucket*>(entry.m_size)->GetSlotSize();
				break;
			}

			default:
				size = entry.m_size;
				m_allocated -= size;
				g_allocated -= size;
		}

		m_memTracker->EraseAllocatorEntry(index);	
		if (m_isGlobal && g_logging)
		{
			DisplayTime();
			std::cout << "freed "; 
			ReportSize(size);
			std::cout << "at: " << 
			ptr << " - total memory: ";
			ReportSize(g_allocated); 
			std::cout << std::endl;
		}
		else
		{
			if (t_logging)
			{
				DisplayTime();
				DisplayThread();
				std::cout << "freed "; 
				ReportSize(size);
				std::cout << "at: " << 
				ptr << " - thread_memory: ";
				ReportSize(m_allocated);
				std::cout << ", total memory: ";
				ReportSize(g_allocated);
				std::cout << std::endl;
			}
		}
	}
	return entry;
}

void MemManager::SetName(const char* name)
{
	m_name = name;
	if ((m_isGlobal && g_logging) || (!m_isGlobal && t_logging))
	{
		std::cout << "Thread " << m_tid << " is now named ";
		DisplayThread();
		std::cout << std::endl;
	}
}

void MemManager::AddBucket(std::size_t size)
{
	if (m_isGlobal)
	{
		const std::lock_guard<std::mutex>lock(m_bucket_mutex);
		m_buckets.emplace_back(new Bucket(size,true));
		if (g_logging)
		{
			std::cout << "Global Bucket Added for size: ";
			ReportSize(size);
			std::cout << std::endl;	
		}
	}
	else
	{
		m_buckets.emplace_back(new Bucket(size, false));
		if (t_logging)
		{
			std::cout << "Thread "; 
			DisplayThread();
			std::cout << "Bucket Added for size: ";
			ReportSize(size);
			std::cout << std::endl;
		}
	}
}

std::size_t MemManager::AddBin(std::size_t size)
{
	std::size_t result;
	if (m_isGlobal)
	{
		const std::lock_guard<std::mutex>lock(m_bin_mutex);
		result = m_bins.size();
		m_bins.emplace_back(new Bin(size,true));
		if (g_logging)
		{
			std::cout << "Global Bin: " << result << " Added of size: ";
			ReportSize(size);
			std::cout << std::endl;	
		}
	}
	else
	{
		result = m_bins.size();
		m_bins.emplace_back(new Bin(size, false));
		if (t_logging)
		{
			std::cout << "Thread "; 
			DisplayThread();
			std::cout << "Bin: " << result << " Added of size: ";
			ReportSize(size);
			std::cout << std::endl;
		}
	}
	return result;
}

