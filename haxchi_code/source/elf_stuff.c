#include "elf_stuff.h"

int strcmp(const char *s1, const char *s2)
{
	while(*s1 && *s2)
	{
		if(*s1 != *s2)
			return -1;
		
		s1++;
		s2++;
	}

	if(*s1 != *s2)
		return -1;

	return 0;
}

uint32_t get_section(uint8_t *data, char *name, uint32_t *size, uint32_t *addr)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)data;
	Elf32_Shdr *shdr = (Elf32_Shdr *) (data + ehdr->e_shoff);

	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		const char *section_name = ((const char*)data) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
		if(strcmp(section_name, name) == 0)
		{

			if(addr)
				*addr = shdr[i].sh_addr;
			if(size)
				*size = shdr[i].sh_size;
			return shdr[i].sh_offset;
		}
	}

	return 0;
}

uint8_t *install_section(uint8_t *data, char *name)
{
	uint32_t section_addr = 0;
	uint32_t section_len = 0;
	uint32_t section_offset = get_section(data, name, &section_len, &section_addr);
	if(section_offset > 0)
	{
		uint8_t *section = data + section_offset;
		/* Copy data to memory */
		Syscall_0x25((void*)(0xC0000000 + section_addr), section, section_len);
	}

	return (uint8_t*)section_addr;

}