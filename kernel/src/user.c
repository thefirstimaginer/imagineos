#include "user.h"
#include <stddef.h>
#include "print.h"

#define MULTIBOOT_TAG_END 0
#define MULTIBOOT_TAG_MODULE 3
#define USER_IMAGE_BASE 0x400000u
#define USER_STACK_TOP  0x5FF000u

typedef struct {
    uint32_t type;
    uint32_t size;
} MultibootTag;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t start;
    uint32_t end;
    char name[0];
} MultibootModuleTag;

typedef struct {
    unsigned char magic[4];
    uint8_t class;
    uint8_t data;
    uint8_t version;
    uint8_t osabi;
    uint8_t abi_version;
    uint8_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version2;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
} ElfHeader;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
} ElfProgramHeader;

extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
static uint64_t multiboot_info_address;

static int module_name_matches(const char *module_name, const char *wanted_name) {
    while (*wanted_name != '\0') {
        if (*module_name++ != *wanted_name++) return 0;
    }
    return *module_name == '\0';
}

static int load_elf(uint32_t start, uint32_t end, uint64_t *entry) {
    ElfHeader *header = (ElfHeader *)(uintptr_t)start;
    unsigned int index;
    if (end <= start || end - start < sizeof(ElfHeader)) return -1;
    if (header->magic[0] != 0x7F || header->magic[1] != 'E' ||
        header->magic[2] != 'L' || header->magic[3] != 'F' ||
        header->class != 2 || header->machine != 0x3E) return -1;
    for (index = 0; index < header->phnum; index++) {
        ElfProgramHeader *program = (ElfProgramHeader *)((uintptr_t)header +
            header->phoff + index * header->phentsize);
        if (program->type != 1) continue;
        if (program->virtual_address < USER_IMAGE_BASE ||
            program->file_size > program->memory_size ||
            program->offset + program->file_size > (uint64_t)(end - start)) return -1;
        memcpy((void *)(uintptr_t)program->virtual_address,
               (void *)((uintptr_t)header + program->offset),
               (size_t)program->file_size);
        memset((void *)(uintptr_t)(program->virtual_address + program->file_size),
               0, (size_t)(program->memory_size - program->file_size));
    }
    *entry = header->entry;
    return 0;
}

static int load_named_module(const char *name, uint64_t *entry) {
    uint32_t offset = 8;
    while (1) {
        MultibootTag *tag = (MultibootTag *)((uintptr_t)multiboot_info_address + offset);
        if (tag->type == MULTIBOOT_TAG_END) break;
        if (tag->type == MULTIBOOT_TAG_MODULE) {
            MultibootModuleTag *module = (MultibootModuleTag *)tag;
            if (module_name_matches(module->name, name)) {
                return load_elf(module->start, module->end, entry);
            }
        }
        offset += (tag->size + 7) & ~7u;
    }
    return -1;
}

int user_init_from_multiboot(uint64_t multiboot_info) {
    uint64_t entry;

    multiboot_info_address = multiboot_info;
    print_str("[INFO] loading userspace module init.elf\n");
    if (load_named_module("init.elf", &entry) == 0) {
        user_enter(entry, USER_STACK_TOP);
    }
    print_str("[FAIL] invalid init.elf\n");
    return -1;
}

int user_exec_service(const char *service_name) {
    uint64_t entry;

    if (module_name_matches(service_name, "shell.service")) {
        if (load_named_module("shell.elf", &entry) != 0) return -1;
    } else {
        return -1;
    }
    user_enter(entry, USER_STACK_TOP);
    return -1;
}