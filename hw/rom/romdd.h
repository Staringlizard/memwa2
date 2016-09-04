#ifndef _ROMDD_H
#define _ROMDD_H

typedef enum
{
	ROM_DD_SECTION_DOS
} rom_dd_section_t;

unsigned char *rom_dd_get_memory(rom_dd_section_t rom_section);

#endif
