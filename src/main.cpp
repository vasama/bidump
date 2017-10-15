#include "bidmp.hpp"

#include <boost/program_options.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::experimental::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::variables_map varmap;

	try
	{
		po::options_description options("Usage: bidump <dump file>");
		options.add_options()
			("help,h", "Display help message")
			("input-file", po::value<std::vector<fs::path>>(), "Crash dump file")
		;

		po::positional_options_description args;
		args.add("input-file", -1);

		po::store(
			po::command_line_parser(argc, argv)
				.options(options)
				.positional(args)
				.run(),
			varmap
		);

		if (varmap.count("help"))
		{
			std::cout << options;
			return 0;
		}
	}
	catch (po::error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	if (varmap.count("input-file") == 0)
	{
		std::cerr << "Expected file name" << std::endl;
		return 1;
	}

	for (auto& path : varmap["input-file"].as<std::vector<fs::path>>())
	{
		if (!fs::exists(path))
		{
			std::cerr << "File '" << path << "' not found";
			return 1;
		}

		if (path.extension() == ".bidmp")
		{
			std::vector<std::byte> data;
			{
				typedef std::uint8_t int_type;
				std::basic_ifstream<int_type> stream(
					path, std::ios::binary);
				if (stream.fail())
				{
					std::cerr << "Cannot open file";
					return 1;
				}
				std::array<int_type, 1024> buffer;
				std::streamsize size;

				while (!stream.eof())
				{
					stream.read(buffer.data(), buffer.size());
					auto size = stream.gcount();
					if (stream.fail() && !stream.eof())
					{
						std::cerr << "Error reading file";
						return 1;
					}
					auto offset = data.size();
					data.resize(offset + size);
					std::memcpy(data.data() + offset,
						buffer.data(), size);
				}
			}
			
			std::cout << path << ":\n";
			if (auto bidmp = read_bidmp(data.data(), data.size()))
			{
				print_bidmp(std::cout, *bidmp);
				std::cout << "\n";
			}
			else
			{
				std::cerr << "Error reading crash dump" << std::endl;
				return 1;
			}
		}
	}
}