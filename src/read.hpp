#pragma once

#include <boost/endian/buffers.hpp>

#include <cstddef>

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, std::byte& out)
{
	if (first == last)
		return false;
	out = *first++;
	return true;
}

template<typename Iterator, typename T
#if !defined(__INTELLISENSE__)
	, typename std::enable_if_t<std::is_integral_v<T>>* = nullptr
#endif
>
bool read(Iterator& first, const Iterator& last, T& out)
{
	typedef boost::endian::endian_buffer<
		boost::endian::order::little,
		T, sizeof(T) * 8,
		boost::endian::align::no
	> buf_t;

	if (last - first < sizeof(T))
		return false;

	const std::byte* data = &*first;
	out = reinterpret_cast<const buf_t*>(data)->value();

	first += sizeof(T);
	return true;
}

template<typename Iterator, typename OutputIterator>
bool read(Iterator& first, const Iterator& last,
	OutputIterator out_first, OutputIterator out_last)
{
	Iterator save = first;
	while (out_first != out_last)
	{
		if (first == last)
		{
			first = save;
			return false;
		}

		read(first, last, *out_first++);
	}
	return true;
}
