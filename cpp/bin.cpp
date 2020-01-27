#include "bin.h"

Bin::Bin(std::size_t size, bool m_isGlobal)
: m_capacity(size)
, m_freeSpace(size)
{
	
}