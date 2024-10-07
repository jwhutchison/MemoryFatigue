#pragma once

#include <cstdint>

struct DosHeader {
	uint16_t magic;
	uint8_t ignored[58];
	int32_t coff_header_offset;
};

struct CoffHeader {
	uint32_t signature;
	uint16_t machine;
	uint16_t number_of_sections;
	uint32_t time_date_stamp;
	uint32_t pointer_to_symbol_table;
	uint32_t number_of_symbols;
	uint16_t size_of_optional_header;
	uint16_t characteristics;
};

struct CoffOptionalHeader {
	uint16_t magic;
	uint8_t ignored[104];
	uint16_t number_of_rva_and_sizes;
};

struct SectionHeader {
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint8_t ignored[24];
};