#ifndef _ROMCC_H
#define _ROMCC_H

typedef enum
{
	ROM_CC_SECTION_KROM,
	ROM_CC_SECTION_BROM,
	ROM_CC_SECTION_CROM
} rom_cc_section_t;

unsigned char *rom_cc_get_memory(rom_cc_section_t rom_section);

#endif
