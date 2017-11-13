#pragma once

#include <memory>
#include <ostream>
#include <cstddef>
#include <cstdint>

struct bidmp;

struct bidmp_delete
{
	void operator()(bidmp*);
};

typedef std::unique_ptr<bidmp, bidmp_delete> bidmp_ptr;

bidmp_ptr read_bidmp(const std::byte* data, std::size_t size);

std::ostream& print_bidmp(std::ostream& os, const bidmp& bidmp, std::uint32_t known_func);
