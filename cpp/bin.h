#pragma once

#include <memory>
#include <mutex>

class Bin
{
public: 
	Bin(std::size_t size, bool m_isGlobal);
	std::size_t QueryFreeSpace() const { return 0; }
	bool WillFit() const { return true; }

private:
	Bin();
	std::size_t m_capacity;
	std::size_t m_freeSpace;

	std::unique_ptr<unsigned char> m_buffer;
	std::unique_ptr<std::mutex> m_bin_mutex;
};
