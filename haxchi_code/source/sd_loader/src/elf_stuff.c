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

#define COREINIT(x) (x - 0xFE3C00) // From IDA
void (*memcpy)(void *, void *, int);
void (*memset)(void *, int, int);
void (*DCFlushRange)(void *buffer, uint32_t length);
void (*ICInvalidateRange)(void *buffer, uint32_t length);

uint32_t load_elf_image(uint8_t *data)
{

	memcpy = (void*)COREINIT(0x02019BC8);
	memset = (void*)COREINIT(0x02019BB4);
	DCFlushRange = (void*)COREINIT(0x02007B88);
	ICInvalidateRange = (void*)COREINIT(0x02007CB0);
	
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdrs;
	uint8_t *image;
	int i;

	ehdr = (Elf32_Ehdr *) data;

	if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
		return 0;

	if(ehdr->e_phentsize != sizeof(Elf32_Phdr))
		return 0;

	phdrs = (Elf32_Phdr*)(data + ehdr->e_phoff);

	for(i = 0; i < ehdr->e_phnum; i++)
	{
		if(phdrs[i].p_type != PT_LOAD)
			continue;

		if(phdrs[i].p_filesz > phdrs[i].p_memsz)
			continue;

		if(!phdrs[i].p_filesz)
			continue;

		uint32_t p_paddr = phdrs[i].p_paddr;
		image = (uint8_t*) (data + phdrs[i].p_offset);

		memcpy((void *)p_paddr, image, phdrs[i].p_filesz);
		DCFlushRange((void*)p_paddr, phdrs[i].p_filesz);
		ICInvalidateRange((void *)p_paddr, phdrs[i].p_memsz);
	}

	Elf32_Shdr *shdr = (Elf32_Shdr *) (data + ehdr->e_shoff);
	for(i = 0; i < ehdr->e_shnum; i++)
	{
		const char *section_name = ((const char*)data) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
		if((strcmp(section_name, ".bss") == 0) || (strcmp(section_name, ".sbss") == 0))
		{
			memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
			DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
		}
	}

	return ehdr->e_entry;
}