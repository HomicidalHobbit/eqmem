
#include "bucket.h"
#include <iostream>

#define BUCKET_SLOTS 256
#define MAX_BUCKET_ENTRY_SIZE 256

Bucket::Bucket(std::size_t size, bool isGlobal)
: m_buffer(new unsigned char[size * BUCKET_SLOTS])
, m_slot_size(size)	
, m_capacity(BUCKET_SLOTS)
, m_free(BUCKET_SLOTS)
, m_isGlobal(isGlobal)
{
	m_freelist.resize(BUCKET_SLOTS);
	for (std::size_t i = 0; i < BUCKET_SLOTS; ++i)
	{
		m_freelist[i] = i;
	}
}

void* Bucket::Allocate()
{
	void* ptr = &m_buffer.get()[m_slot_size * m_freelist[0]];
	m_freelist[0] = m_freelist.back();
	m_freelist.pop_back();
	--m_free;
	std::cout << "Adding to Bucket at: " << ptr << std::endl;
	return ptr;
}

void Bucket::Deallocate(void* ptr)
{
	if (m_isGlobal)
	{
		const std::lock_guard<std::mutex>lock(*m_bucket_mutex);
		std::size_t index = (reinterpret_cast<unsigned char*>(ptr) - m_buffer.get()) / m_slot_size;
		std::cout << "Freeing Index: " << index << std::endl;
		++m_free;
	}
	else
	{
		std::size_t index = (reinterpret_cast<unsigned char*>(ptr) - m_buffer.get()) / m_slot_size;
		std::cout << "Freeing Index: " << index << std::endl;
		++m_free;
	}
}
