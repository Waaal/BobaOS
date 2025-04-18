#ifndef IO_H
#define IO_H

#include <stdint.h>

void outb(uint16_t port, uint8_t out);
void outw(uint16_t port, uint16_t out);
void outd(uint16_t port, uint32_t out);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t ind(uint16_t port);

#endif
