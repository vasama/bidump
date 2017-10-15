#include "bidmp.hpp"

#include "indent.hpp"
#include "read.hpp"

#include <array>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>

#include <cstdint>

#define NOMINMAX
#include <Windows.h>

typedef std::int32_t i32;
typedef std::uint32_t u32;

struct header
{
	i32 version;
	i32 build;
};

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, header& out)
{
	constexpr char Magic[] = "STK7";
	constexpr i32 StkVersion = 103;

	Iterator local = first;

	char magic[sizeof(Magic) / sizeof(*Magic)];
	i32 stkVersion;

	if (!read(local, last, std::begin(magic), std::end(magic)) || strcmp(magic, Magic) != 0)
		return false;

	if (!read(local, last, stkVersion) || stkVersion != StkVersion)
		return false;

	if (!read(local, last, out.version) || !read(local, last, out.build))
		return false;

	first = local;
	return true;
}

struct context : _CONTEXT
{
};

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, context& out)
{
	Iterator save = first;

	i32 ctx_size;
	if (!read(first, last, ctx_size) || ctx_size != sizeof(out) ||
		(std::size_t)last - (std::size_t)first < sizeof(out))
	{
		first = save;
		return false;
	}

	read(first, last, out.ContextFlags);

	read(first, last, out.Dr0);
	read(first, last, out.Dr1);
	read(first, last, out.Dr2);
	read(first, last, out.Dr3);
	read(first, last, out.Dr6);
	read(first, last, out.Dr7);

	_FLOATING_SAVE_AREA& fsa = out.FloatSave;

	read(first, last, fsa.ControlWord);
	read(first, last, fsa.StatusWord);
	read(first, last, fsa.TagWord);
	read(first, last, fsa.ErrorOffset);
	read(first, last, fsa.ErrorSelector);
	read(first, last, fsa.DataOffset);
	read(first, last, fsa.DataSelector);
	read(first, last,
		std::begin(fsa.RegisterArea),
		std::end(fsa.RegisterArea)
	);
	read(first, last, fsa.Spare0);

	read(first, last, out.SegGs);
	read(first, last, out.SegFs);
	read(first, last, out.SegEs);
	read(first, last, out.SegDs);

	read(first, last, out.Edi);
	read(first, last, out.Esi);
	read(first, last, out.Ebx);
	read(first, last, out.Edx);
	read(first, last, out.Ecx);
	read(first, last, out.Eax);

	read(first, last, out.Ebp);
	read(first, last, out.Eip);
	read(first, last, out.SegCs);
	read(first, last, out.EFlags);
	read(first, last, out.Esp);
	read(first, last, out.SegSs);

	read(first, last,
		std::begin(out.ExtendedRegisters),
		std::end(out.ExtendedRegisters)
	);

	return true;
}

struct info
{
	std::string file;
	std::string source;
	i32 size;
};

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, info& out)
{
	Iterator local = first;

	i32 length;
	if (!read(local, last, length))
		return false;

	out.file.resize(length);
	if (!read(local, last, out.file.begin(), out.file.end()))
		return false;
	if (out.file.back() == 0)
		out.file.pop_back();

	if (!read(local, last, length))
		return false;

	out.source.resize(length);
	if (!read(local, last, out.source.begin(), out.source.end()))
		return false;
	if (out.file.back() == 0)
		out.file.pop_back();

	if (!read(local, last, out.size))
		return false;

	char ext[2];
	if (!read(local, last, std::begin(ext), std::end(ext)))
		return false;

	first = local;
	return true;
}

struct code
{
	u32 base;
	i32 size;
	u32 func;

	std::vector<
		u32
	> crc;
};

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, code& out)
{
	constexpr std::size_t PageSize = 4096;

	Iterator local = first;

	if (!read(local, last, out.base) ||
		!read(local, last, out.size) ||
		!read(local, last, out.func))
		return false;

	out.crc.resize(out.size / PageSize);
	if (!read(local, last, out.crc.begin(), out.crc.end()))
		return false;

	first = local;
	return true;
}

struct stack
{
	u32 base;
	i32 size;

	std::vector<
		std::byte
	> data;
};

template<typename Iterator>
bool read(Iterator& first, const Iterator& last, stack& out)
{
	Iterator local = first;

	if (!read(local, last, out.base) ||
		!read(local, last, out.size))
		return false;

	out.data.resize(out.size);
	if (!read(local, last, out.data.begin(), out.data.end()))
		return false;

	first = local;
	return true;
}

typedef std::tuple<std::string_view, i32, i32> VersionIdentifier;
const std::map<VersionIdentifier, u32> KnownFunctionOffsets = {
		{ { "arma2oaserver.exe", 163, 131129 }, 0x00187177 },
	};

struct bidmp
{
	header header;
	context context;
	info info;
	code code;
	stack stack;
};

void bidmp_delete::operator()(bidmp* ptr)
{
	delete ptr;
}

bidmp_ptr read_bidmp(const std::byte* data, std::size_t size)
{
	bidmp_ptr ptr(new bidmp());

	const std::byte* last = data + size;

	if (!read(data, last, ptr->header))
		return {};

	if (!read(data, last, ptr->context))
		return {};

	if (!read(data, last, ptr->info))
		return {};

	if (!read(data, last, ptr->code))
		return {};

	if (!read(data, last, ptr->stack))
		return {};

	if (data != last)
		return {};

	return ptr;
}

struct ptr
{
	u32 addr;

	friend std::ostream& operator<<(std::ostream& os, const ptr& p)
	{
		std::array<char, 9> buffer;
		std::snprintf(buffer.data(), buffer.size(), "%08X", p.addr);
		return os << std::string_view(buffer.data(), buffer.size() - 1);
	}
};

std::ostream& print_bidmp(std::ostream& os, const bidmp& b)
{
	constexpr char lf = '\n';

	indent i("  ");

	os << i << b.info.file << " " << b.header.version << "." << b.header.build << lf << lf;

	os << i << "Registers:" << lf;
	++i;
	os << std::hex;
	os << i << "EAX: " << ptr{ b.context.Eax } << lf;
	os << i << "EBX: " << ptr{ b.context.Ebx } << lf;
	os << i << "ECX: " << ptr{ b.context.Ecx } << lf;
	os << i << "EDX: " << ptr{ b.context.Edx } << lf;
	os << i << "EDI: " << ptr{ b.context.Edi } << lf;
	os << i << "EBP: " << ptr{ b.context.Ebp } << lf;
	os << i << "ESP: " << ptr{ b.context.Esp } << lf;
	os << i << "EIP: " << ptr{ b.context.Eip } << lf;
	--i;
	os << lf;

	os << i << "Flags:" << lf;
	++i;
	os << i << "CF: " << (bool)(b.context.EFlags & 0b10000000000000000000000000000000) << lf;
	os << i << "PF: " << (bool)(b.context.EFlags & 0b00100000000000000000000000000000) << lf;
	os << i << "AF: " << (bool)(b.context.EFlags & 0b00001000000000000000000000000000) << lf;
	os << i << "ZF: " << (bool)(b.context.EFlags & 0b00000010000000000000000000000000) << lf;
	os << i << "SF: " << (bool)(b.context.EFlags & 0b00000001000000000000000000000000) << lf;
	os << i << "TF: " << (bool)(b.context.EFlags & 0b00000000100000000000000000000000) << lf;
	os << i << "IF: " << (bool)(b.context.EFlags & 0b00000000010000000000000000000000) << lf;
	os << i << "DF: " << (bool)(b.context.EFlags & 0b00000000001000000000000000000000) << lf;
	os << i << "OF: " << (bool)(b.context.EFlags & 0b00000000000100000000000000000000) << lf;
	--i;
	os << lf;

	i32 adjust = 0;
	{
		auto it = KnownFunctionOffsets.find(
			{ b.info.file, b.header.version, b.header.build });
		if (it != KnownFunctionOffsets.end())
		{
			u32 func = it->second;
			adjust = (i32)b.code.func - (i32)func;
		}
	}

	u32 text_base = adjust == 0 ? 0 : b.code.base;
	u32 text_size = adjust == 0 ? 0 : b.code.size;
	
	os << i << "Stack:" << lf;
	++i;
	auto stack_first = b.stack.data.data(),
		stack_last = b.stack.data.data() + b.stack.data.size();
	u32 addr = b.stack.base + b.stack.size;
	while (stack_first != stack_last)
	{
		u32 value;
		if (!read(stack_first, stack_last, value))
			break;

		os << i << ptr{ addr } << " " << ptr{ value };
		if (value >= text_base && value < text_base + text_size)
			os << " " << b.info.file << "+" << ptr{ value + adjust };
		os << lf;

		addr -= sizeof(u32);
	}
	--i;

	return os;
}
