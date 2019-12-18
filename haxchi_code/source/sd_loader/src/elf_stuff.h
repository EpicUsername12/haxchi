#ifndef ELF_STUFF_H
#define ELF_STUFF_H

#include "../../elf_abi.h"
#include "../../types.h"

uint32_t load_elf_image(uint8_t *data);
uint32_t get_section(uint8_t *data, char *name, uint32_t *size, uint32_t *addr);
uint8_t *install_section(uint8_t *data, char *name);
#endif