#pragma once

#include <algorithm>
#include <ostream>
#include <string>
#include <string_view>

class indent
{
	std::string m_indent;
	int m_depth = 0;

public:
	indent(std::string indent)
		: m_indent(indent)
	{
	}

	int depth() const
	{
		return m_depth;
	}

	indent& operator+=(int i)
	{
		m_depth += i;
		return *this;
	}

	indent& operator-=(int i)
	{
		m_depth = std::max(m_depth - i, 0);
		return *this;
	}

	indent& operator++()
	{
		return (*this) += 1;
	}

	indent& operator--()
	{
		return (*this) -= 1;
	}

	friend std::ostream& operator<<(std::ostream& os, const indent& indent)
	{
		std::string_view view = indent.m_indent;
		for (int i = 0, d = indent.m_depth; i < d; ++i)
			os << view;
		return os;
	}
};