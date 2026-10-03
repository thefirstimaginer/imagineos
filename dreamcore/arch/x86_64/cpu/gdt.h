#pragma once

#define GDT_RING_0 0
#define GDT_RING_1 1 // legacy
#define GDT_RING_2 2 // legacy
#define GDT_RING_3 3

#define GDT_SELECTOR_CS_KERNEL 0x08
#define GDT_SELECTOR_DS_KERNEL 0x10
#define GDT_SELECTOR_DS_USER 0x18
#define GDT_SELECTOR_CS_USER 0x20
