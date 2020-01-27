#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

class Bucket
{
public:

	Bucket(std::size_t size, bool m_isGlobal);
	const std::size_t GetSlotSize() const { return m_slot_size; }
	const std::size_t GetFree() const { return m_free; }
	const std::size_t GetCapacity() const { return m_capacity; }
	const std::size_t GetSize() const { return m_capacity * m_slot_size; }
	void* Allocate();
	void Deallocate(void* ptr);

private:
	Bucket();
	std::unique_ptr<unsigned char> m_buffer;
	std::size_t m_slot_size;
	std::size_t m_capacity;
	std::size_t m_free;
	std::vector<std::size_t> m_freelist;
	std::unique_ptr<std::mutex> m_bucket_mutex;
	bool m_isGlobal;
};
