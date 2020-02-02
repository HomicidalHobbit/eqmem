#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "memtracker.h"

//#include "eqmem.h"

#define THREADSAFE true

class Bin;
class Bucket;
class MemTracker;

// Hashing algorithm merely returns the ptr value as a std::size_t, as it already represents a unique hash
struct PtrHash
{
	std::size_t operator()(void* const& ptr) const noexcept
	{
		return reinterpret_cast<std::size_t>(ptr);
	}

};

class MemManager
{
public:
	MemManager(bool threadsafe = false);
	~MemManager();
	void* Allocate(std::size_t size, int tag, Allocator allocator);
	void AddBucket(std::size_t size);
	std::size_t AddBin(std::size_t size);
	Bucket* FindBucket(std::size_t size);
	AllocatorEntry Deallocate(void* ptr);
	void SetName(const char* name); 
	void DisplayThread();
	void DisplayTime();
	void ReportSize(std::size_t size);
	const std::size_t GetThreadID() const { return m_id; } 
	const std::size_t TotalAllocated() const { return m_allocated; }
	const std::string& GetName() const { return m_name; }
	void DisplayAllocations();	
	static int SetTags(const char* name);
	static void SetLeakWarning(bool enable);

private:
	MemTracker* m_memTracker;
	std::vector<std::unique_ptr<Bucket>> m_buckets;
	std::vector<std::unique_ptr<Bin>> m_bins;
	std::mutex m_bin_mutex;
	std::mutex m_bucket_mutex;
	std::thread::id m_tid;
	std::size_t m_id;
	std::string m_name;
	std::size_t m_allocated;
	bool m_isGlobal;
	bool m_used;
};

class ThreadMemManager
{
public:
	ThreadMemManager();
	~ThreadMemManager();
	MemManager* GetMemManager() { return m_memManager; }

private:
	MemManager* m_memManager;
};

extern std::mutex* g_global_mutex;
extern MemManager g_memManager;
extern std::vector<std::string>* g_tags;
extern bool g_logging;
extern std::mutex* g_transient_mutex;
extern std::vector<void*> g_transients;
extern std::atomic<bool> g_transients_waiting;
extern thread_local std::unique_ptr<ThreadMemManager> t_threadMemManager;
extern thread_local bool t_logging;

