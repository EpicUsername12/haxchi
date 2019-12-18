#ifndef ELF_STUFF_H
#define ELF_STUFF_H

#include "elf_abi.h"
#include "types.h"
#include <string.h>

extern void Syscall_0x25(void* addr, void* src, unsigned int len);

uint32_t get_section(uint8_t *data, char *name, uint32_t *size, uint32_t *addr);
uint8_t *install_section(uint8_t *data, char *name);
#endif