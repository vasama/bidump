#include "bidmp.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cctype>

typedef int32_t i32;
typedef uint32_t u32;

static bool read(const std::byte*& first, const std::byte* last, std::byte& out)
{
	if (first == last)
		return false;
	out = *first++;
	return true;
}

template<typename T>
static auto read(const std::byte*& first, const std::byte* last, T& out)
	-> std::enable_if_t<std::is_integral_v<T>, bool>
{
	if (last - first < sizeof(T))
		return false;

	memcpy(&out, first, sizeof(T));

	first += sizeof(T);
	return true;
}

template<typename OutputIterator>
static bool read(const std::byte*& first, const std::byte* last,
	OutputIterator out_first, OutputIterator out_last)
{
	const std::byte* save = first;
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

struct header
{
	i32 version;
	i32 build;
};

static bool read(const std::byte*& first, const std::byte* last, header& out)
{
	constexpr char Magic[] = "STK7";
	constexpr i32 StkVersion = 103;

	const std::byte* local = first;

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

struct context
{
	struct floating_save_area
	{
		u32 control_word;
		u32 status_word;
		u32 tag_word;
		u32 error_offset;
		u32 error_selector;
		u32 data_offset;
		u32 data_selector;
		std::byte register_area[80];
		u32 spare0;
	};

	u32 context_flags;

	u32 dr0;
	u32 dr1;
	u32 dr2;
	u32 dr3;
	u32 dr6;
	u32 dr7;

	floating_save_area fsa;

	u32 seg_gs;
	u32 seg_fs;
	u32 seg_es;
	u32 seg_ds;

	u32 edi;
	u32 esi;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;

	u32 ebp;
	u32 eip;
	u32 seg_cs;
	u32 eflags;
	u32 esp;
	u32 seg_ss;

	std::byte extended_registers[512];
};

static bool read(const std::byte*& first, const std::byte* last, context& out)
{
	const std::byte* save = first;

	i32 ctx_size;
	if (!read(first, last, ctx_size) || ctx_size != sizeof(out))
	{
		first = save;
		return false;
	}

	if (!read(first, last, (std::byte*)&out, (std::byte*)&out + sizeof(out)))
	{
		first = save;
		return false;
	}

	return true;

	/*read(first, last, out.context_flags);

	read(first, last, out.dr0);
	read(first, last, out.dr1);
	read(first, last, out.dr2);
	read(first, last, out.dr3);
	read(first, last, out.dr6);
	read(first, last, out.dr7);

	auto& fsa = out.fsa;

	read(first, last, fsa.control_word);
	read(first, last, fsa.status_word);
	read(first, last, fsa.tag_word);
	read(first, last, fsa.error_offset);
	read(first, last, fsa.error_selector);
	read(first, last, fsa.data_offset);
	read(first, last, fsa.data_selector);
	read(first, last,
		std::begin(fsa.register_area),
		std::end(fsa.register_area)
	);
	read(first, last, fsa.spare0);

	read(first, last, out.seg_gs);
	read(first, last, out.seg_fs);
	read(first, last, out.seg_es);
	read(first, last, out.seg_ds);

	read(first, last, out.edi);
	read(first, last, out.esi);
	read(first, last, out.ebx);
	read(first, last, out.edx);
	read(first, last, out.ecx);
	read(first, last, out.eax);

	read(first, last, out.ebp);
	read(first, last, out.eip);
	read(first, last, out.seg_cs);
	read(first, last, out.eflags);
	read(first, last, out.esp);
	read(first, last, out.seg_ss);

	read(first, last,
		std::begin(out.extended_registers),
		std::end(out.extended_registers)
	);

	first = save + ctx_size;*/
}

struct info
{
	std::string file;
	std::string source;
	i32 size;
};

static bool read(const std::byte*& first, const std::byte* last, info& out)
{
	const std::byte* local = first;

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

static bool read(const std::byte*& first, const std::byte* last, code& out)
{
	constexpr std::size_t PageSize = 4096;

	const std::byte* local = first;

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

	std::vector<
		std::byte
	> data;
};

static bool read(const std::byte*& first, const std::byte* last, stack& out)
{
	const std::byte* local = first;

	i32 size;
	if (!read(local, last, out.base) ||
		!read(local, last, size))
		return false;

	out.data.resize(size);
	if (!read(local, last, out.data.begin(), out.data.end()))
		return false;

	first = local;
	return true;
}

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


typedef std::tuple<std::string_view, i32, i32> version_identifier;
const std::map<version_identifier, u32> known_function_offsets = {
	{ { "arma2oaserver.exe", 163, 131129 }, 0x00187177 },
	{ { "arma2oaserver.exe", 164, 144629 }, 0x00131c4b },
	{ { "arma2oa.exe", 163, 131129 }, 0x00282476 },
	{ { "arma2oa.exe", 164, 144629 }, 0x002c7691 }
};

const u32* get_known_function_offset(std::string_view file, i32 version, i32 build)
{
	auto file_low = std::string{ file };
	for (auto& x : file_low)
		x = (std::string::value_type)std::tolower(x);
	auto it = known_function_offsets.find({ file_low, version, build });
	if (it != known_function_offsets.end()) return &(it->second);
	return nullptr;
}

class ptr
{
	u32 m_addr;

public:
	ptr(u32 addr)
		: m_addr(addr)
	{
	}
	
	friend std::ostream& operator<<(std::ostream& os, const ptr& p)
	{
		std::array<char, 9> buffer;
		std::snprintf(buffer.data(), buffer.size(), "%08X", p.m_addr);
		return os << std::string_view(buffer.data(), buffer.size() - 1);
	}
};

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

std::ostream& print_bidmp(std::ostream& os, const bidmp& b, std::uint32_t known_func)
{
	constexpr char lf = '\n';

	indent i("  ");

	os << i << b.info.file << " " << b.header.version << "." << b.header.build;

	i32 adjust = 0;
	{
		const u32* p_rva = get_known_function_offset(
			b.info.file, b.header.version, b.header.build);
		u32 rva = p_rva ? *p_rva : known_func;
		if (rva != 0)
		{
			adjust = (i32)b.code.func - (i32)rva;
			os << " @ " << ptr{ (u32)adjust };
		}
	}
	os << lf << "known function address: " << ptr{ b.code.func } << lf << lf;

	u32 text_base = adjust == 0 ? 0 : b.code.base;
	u32 text_size = adjust == 0 ? 0 : b.code.size;

	auto const print_rva = [&](u32 value) {
		if (value >= text_base && value < text_base + text_size)
			os << " " << b.info.file << "+" << ptr{ value - adjust };
	};

	os << i << "Registers:" << lf;
	++i;
	os << std::hex;
	os << i << "EAX: " << ptr{ b.context.eax }; print_rva(b.context.eax); os << lf;
	os << i << "EBX: " << ptr{ b.context.ebx }; print_rva(b.context.ebx); os << lf;
	os << i << "ECX: " << ptr{ b.context.ecx }; print_rva(b.context.ecx); os << lf;
	os << i << "EDX: " << ptr{ b.context.edx }; print_rva(b.context.edx); os << lf;
	os << i << "ESI: " << ptr{ b.context.esi }; print_rva(b.context.esi); os << lf;
	os << i << "EDI: " << ptr{ b.context.edi }; print_rva(b.context.edi); os << lf;
	os << i << "EBP: " << ptr{ b.context.ebp }; print_rva(b.context.ebp); os << lf;
	os << i << "ESP: " << ptr{ b.context.esp }; print_rva(b.context.esp); os << lf;
	os << i << "EIP: " << ptr{ b.context.eip }; print_rva(b.context.eip); os << lf;
	--i;
	os << lf;

	os << i << "Flags:" << lf;
	++i;
	os << i << "CF: " << (bool)(b.context.eflags & (1u <<  0)) << lf;
	os << i << "PF: " << (bool)(b.context.eflags & (1u <<  2)) << lf;
	os << i << "AF: " << (bool)(b.context.eflags & (1u <<  4)) << lf;
	os << i << "ZF: " << (bool)(b.context.eflags & (1u <<  6)) << lf;
	os << i << "SF: " << (bool)(b.context.eflags & (1u <<  7)) << lf;
	os << i << "TF: " << (bool)(b.context.eflags & (1u <<  8)) << lf;
	os << i << "IF: " << (bool)(b.context.eflags & (1u <<  9)) << lf;
	os << i << "DF: " << (bool)(b.context.eflags & (1u << 10)) << lf;
	os << i << "OF: " << (bool)(b.context.eflags & (1u << 11)) << lf;
	--i;
	os << lf;

	auto stack_first = b.stack.data.data(),
		stack_last = b.stack.data.data() + b.stack.data.size();
	u32 addr = b.stack.base - (u32)b.stack.data.size();
	
	os << i << "Stack:" << lf;
	++i;
	while (stack_first != stack_last)
	{
		u32 value;
		if (!read(stack_first, stack_last, value))
			break;

		os << i << ptr{ addr } << " " << ptr{ value };
		print_rva(value);
		os << lf;

		addr += sizeof(u32);
	}
	--i;

	return os;
}
