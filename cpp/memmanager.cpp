#include "memmanager.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "bin.h"
#include "bucket.h"

#define MAX_BUCKET_ENTRY_SIZE 256

thread_local MemManager t_memManager;
thread_local std::thread::id t_tid;
thread_local bool t_logging(false);

bool g_logging(false);
bool g_leakWarning(true);
MemManager g_memManager(THREADSAFE);
std::atomic<std::size_t> g_allocated(0);
std::atomic<std::size_t> g_managerCount(0);
std::atomic<bool> g_initialised(false);
std::vector<std::string> g_allocators;
std::mutex* g_global_mutex = nullptr;
std::vector<std::string>* g_tags = nullptr;

MemManager::MemManager(bool threadsafe)
: m_isGlobal(threadsafe)
{	
	m_allocated = 0;
	m_used = false;
	m_tid = std::this_thread::get_id();
	t_tid = m_tid;
	std::stringstream ss;
	ss << m_tid;
	m_id = (std::stoul(ss.str(), nullptr, 16));
	if (!g_initialised)
	{
		g_initialised = true;
		std::cout << "Starting EQMem!\n";
		g_global_mutex = new std::mutex;
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
	if (m_used && g_leakWarning)
	{
		if (!m_map.empty())
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
	
			for (const auto& ptr : m_map)
			{
				std::size_t size;
				switch (ptr.second.m_allocator)
				{
					case Default_Bucket:
					{
						size = reinterpret_cast<Bucket*>(ptr.second.m_size)->GetSlotSize();
						break;
					}

					case Local_Bucket:
					{
						size = reinterpret_cast<Bucket*>(ptr.second.m_size)->GetSlotSize();
						break;
					}

					default:
						size = ptr.second.m_size;
				}
				std::cout << "'" << g_tags->at(ptr.second.m_tag) <<
				"' " << ptr.first << " of size ";
				ReportSize(size);
				std::cout << std::endl;
			}
		}
	}
	if (!--g_managerCount)
	{
		delete g_global_mutex;
		delete g_tags;	
		std::cout << "Shutting Down EQMem!" << std::endl;		
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
	if (!ptr)
	{
		ptr = malloc(size);
		m_map[ptr] = AllocatorEntry{ size, tag, allocator};	
		g_allocated += size;
		m_allocated += size;
	}
		
	if (m_isGlobal && g_logging)
	{
		std::cout << "allocated ";
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
			DisplayThread();
			std::cout << "allocated "; 
			ReportSize(size);
			std::cout << "to: " << 
			ptr << " - thread memory: "; 
			ReportSize(m_allocated);
			std::cout << "total memory: "; 
			ReportSize(g_allocated);
			std::cout << std::endl;	
		}
	}
	m_used = true;
	return ptr;
}

void* MemManager::Allocate(const AllocatorEntry& entry)
{
	return nullptr;
}

AllocatorEntry MemManager::Deallocate(void* ptr)
{
	AllocatorEntry entry;
	auto search = m_map.find(ptr);
	if (search == m_map.end())
	{
		entry.m_size = 0;
	}
	else 
	{
		std::size_t size;
		entry = search->second;
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

		m_map.erase(search);	
		if (m_isGlobal && g_logging)
		{
			std::cout << "deallocated "; 
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
				DisplayThread();
				std::cout << "deallocated "; 
				ReportSize(size);
				std::cout << "at: " << 
				ptr << " - thread_memory: ";
				ReportSize(m_allocated);
				std::cout << "total memory: ";
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
		std::cout << "Thread " << m_tid << " named " << m_name << std::endl;
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

