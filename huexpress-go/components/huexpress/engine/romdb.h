#ifndef _ROMFLAGS_H
#define _ROMFLAGS_H

#include <cleantypes.h>

#define TWO_PART_ROM 0x0001
#define US_ENCODED   0x0010
#define USA          0x4000
#define JAP          0x8000

const struct {
	const uint32 CRC;
	const char   *Name;
	const uint32 Flags;
} romFlags[] = {
	{0x00000000, "Unknown", JAP},
	{0xF0ED3094, "Blazing Lazers", USA | TWO_PART_ROM},
	{0xB4A1B0F6, "Blazing Lazers", USA | TWO_PART_ROM},
	{0x55E9630D, "Legend of Hero Tonma", USA | US_ENCODED},
};

#define KNOWN_ROM_COUNT (sizeof(romFlags) / sizeof(romFlags[0]))

#endif /* _ROMFLAGS_H */
