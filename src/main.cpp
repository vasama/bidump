#include "bidmp.hpp"

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace stdfs = std::filesystem;

struct arguments
{
	std::vector<std::string_view> pos;
	std::unordered_map<std::string_view, std::string_view> map;

	arguments(const char* const* args, size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			std::string_view string = args[i];
			if (string.substr(0, 2) == "--")
			{
				string.remove_prefix(2);

				std::string_view value;
				if (size_t eq = string.find('='); eq != string.npos)
				{
					value = string.substr(eq + 1);
					string = string.substr(0, eq);
				}
				else if (i++ < count)
				{
					value = args[i];
				}

				map[string] = value;
			}
			else
			{
				pos.push_back(string);
			}
		}
	}

	bool try_get_arg(std::string_view name, std::string_view* out)
	{
		if (auto it = map.find(name); it != map.end())
		{
			*out = it->second;
			return true;
		}
		return false;
	}
};

template<typename T>
static bool try_parse(std::string_view string, T* out)
{
	auto r = std::from_chars(string.data(), string.data() + string.size(), *out);
	return r.ec == std::errc{} && r.ptr == string.data() + string.size();
}

int main(int argc, char** argv)
{
	arguments args(argv + 1, argc - 1);

	if (args.map.find("help") != args.map.end())
	{
		std::cerr <<
			"Usage: bidump [options...] file\n"
			"    help        display help message\n"
			"    known-addr  relative virtual address of the known function\n";
		return 1;
	}

	uint32_t known_addr = 0;
	if (std::string_view known_addr_string; args.try_get_arg("", &known_addr_string))
	{
		if (!try_parse(known_addr_string, &known_addr))
		{
			std::cerr << "invalid argument: known-addr\n";
			return 1;
		}
	}

	if (args.pos.size() == 0)
	{
		std::cerr << "expected file name\n";
		return 1;
	}

	if (args.pos.size() > 1)
	{
		std::cerr << "unexpected argument: " << args.pos[1] << '\n';
		return 1;
	}

	stdfs::path file_path = args.pos[0];

	if (!stdfs::exists(file_path))
	{
		std::cerr << "file not found: " << file_path << '\n';
		return 1;
	}

	std::basic_ifstream<std::byte> stream(file_path, std::ios::binary);
	std::istreambuf_iterator<std::byte> begin(stream), end;
	std::vector<std::byte> file_data(begin, end);

	if (auto bidmp = read_bidmp(file_data.data(), file_data.size()))
	{
		print_bidmp(std::cout, *bidmp, known_addr);
	}
	else
	{
		std::cerr << "error reading file\n";
		return 1;
	}
}