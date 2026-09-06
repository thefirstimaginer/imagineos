#include "user.h"
#include <stddef.h>
#include "print.h"
#include "paging.h"
#include "process.h"

#define MULTIBOOT_TAG_END 0
#define MULTIBOOT_TAG_MODULE 3
#define USER_IMAGE_BASE 0x400000u
#define USER_STACK_TOP  0x5FF000u

typedef struct {
    uint32_t type;
    uint32_t size;
} MultibootTag;

typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} MultibootInfo;

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

extern void *memset(void *, int, size_t);
static uint64_t multiboot_info_address;
static uint32_t multiboot_info_size;

static int module_name_matches(const char *module_name, const char *wanted_name) {
    while (*wanted_name != '\0') {
        if (*module_name++ != *wanted_name++) return 0;
    }
    return *module_name == '\0';
}

static void copy_segment(void *destination, const void *source, size_t count) {
    unsigned char *to = (unsigned char *)destination;
    const unsigned char *from = (const unsigned char *)source;
    uintptr_t destination_address = (uintptr_t)destination;
    uintptr_t source_address = (uintptr_t)source;

    if (destination_address > source_address &&
        destination_address - source_address < count) {
        to += count;
        from += count;
        while (count != 0) {
            *--to = *--from;
            count--;
        }
        return;
    }

    while (count != 0) {
        *to++ = *from++;
        count--;
    }
}

static int load_elf(uint32_t start, uint32_t end, uint64_t *entry) {
    ElfHeader *header = (ElfHeader *)(uintptr_t)start;
    unsigned int index;
    uint64_t program_headers_end;
    if (end <= start || end - start < sizeof(ElfHeader)) {
        print_str("[FAIL] ELF header bounds\n");
        return -1;
    }
    if (header->magic[0] != 0x7F || header->magic[1] != 'E' ||
        header->magic[2] != 'L' || header->magic[3] != 'F' ||
        header->class != 2 || header->machine != 0x3E) {
        print_str("[FAIL] ELF header format\n");
        return -1;
    }
    if (header->phentsize < sizeof(ElfProgramHeader) ||
        header->phnum > 32 ||
        header->phoff > (uint64_t)(end - start)) {
        print_str("[FAIL] ELF program table\n");
        return -1;
    }
    program_headers_end = header->phoff +
        (uint64_t)header->phentsize * header->phnum;
    if (program_headers_end < header->phoff ||
        program_headers_end > (uint64_t)(end - start)) {
        print_str("[FAIL] ELF program table bounds\n");
        return -1;
    }
    for (index = 0; index < header->phnum; index++) {
        ElfProgramHeader *program = (ElfProgramHeader *)((uintptr_t)header +
            header->phoff + index * header->phentsize);
        if (program->type != 1) continue;
        if (program->virtual_address < USER_IMAGE_BASE ||
            program->virtual_address >= USER_IMAGE_BASE + 0x200000u ||
            program->memory_size > USER_IMAGE_BASE + 0x200000u -
                program->virtual_address ||
            program->file_size > program->memory_size ||
            program->offset > (uint64_t)(end - start) ||
            program->file_size > (uint64_t)(end - start) - program->offset) {
            print_str("[FAIL] ELF segment bounds\n");
            return -1;
        }
        __asm__ volatile ("cli" : : : "memory");
        copy_segment((void *)(uintptr_t)program->virtual_address,
                 (void *)((uintptr_t)header + program->offset),
                 (size_t)program->file_size);
        memset((void *)(uintptr_t)(program->virtual_address + program->file_size),
               0, (size_t)(program->memory_size - program->file_size));
        __asm__ volatile ("sti" : : : "memory");
    }
    *entry = header->entry;
    return 0;
}

static int load_named_module(const char *name, uint64_t *entry) {
    uint32_t offset = 8;
    uint32_t tag_end = multiboot_info_size;

    while (offset + sizeof(MultibootTag) <= tag_end) {
        MultibootTag *tag = (MultibootTag *)((uintptr_t)multiboot_info_address + offset);
        if (tag->type == MULTIBOOT_TAG_END) break;
        if (tag->size < sizeof(MultibootTag) || tag->size > tag_end - offset) {
            print_str("[FAIL] invalid Multiboot tag\n");
            return -1;
        }
        if (tag->type == MULTIBOOT_TAG_MODULE) {
            MultibootModuleTag *module = (MultibootModuleTag *)tag;
            if (tag->size >= sizeof(MultibootModuleTag) &&
                module_name_matches(module->name, name)) {
                return load_elf(module->start, module->end, entry);
            }
        }
        offset += (tag->size + 7) & ~7u;
    }
    print_str("[FAIL] userspace module not found\n");
    return -1;
}

int user_init_from_multiboot(uint64_t multiboot_info) {
    uint64_t entry;
    uint64_t cr3;
    Process *init_process;

    multiboot_info_address = multiboot_info;
    multiboot_info_size = ((MultibootInfo *)(uintptr_t)multiboot_info)->total_size;
    if (multiboot_info_size < 16) {
        print_str("[FAIL] invalid Multiboot information\n");
        return -1;
    }
    print_str("[INFO] loading userspace module init.elf\n");
    if (load_named_module("init.elf", &entry) == 0) {
        print_str("[OK] init.elf loaded, entering userspace\n");
        cr3 = paging_create_user_space();
        if (cr3 == 0) return -1;
        paging_copy_user_image(cr3);
        init_process = process_create_user("init", cr3);
        if (init_process == NULL) return -1;
        if (current_process != NULL) current_process->state = PROCESS_BLOCKED;
        init_process->state = PROCESS_RUNNING;
        current_process = init_process;
        user_enter(entry, USER_STACK_TOP, cr3);
    }
    print_str("[FAIL] invalid init.elf\n");
    return -1;
}

int user_exec_service(const char *service_name) {
    uint64_t entry;
    uint64_t cr3;
    Process *parent;
    Process *child;
    const char *process_name;

    cr3 = paging_create_user_space();
    if (cr3 == 0) return -1;

    if (module_name_matches(service_name, "shell.service")) {
        if (load_named_module("shell.elf", &entry) != 0) return -1;
        process_name = "shell";
    } else if (module_name_matches(service_name, "clear.service")) {
        if (load_named_module("clear.elf", &entry) != 0) return -1;
        process_name = "clear";
    } else {
        return -1;
    }

    /* The ELF is loaded at the fixed 0x400000 virtual address while the kernel
       still owns the active CR3. Switching to the new page table before the copy
       would make the source and destination alias the same user mapping. */
    paging_copy_user_image(cr3);
    parent = current_process;
    child = process_create_user(process_name, cr3);
    if (child == NULL) return -1;
    if (parent != NULL) parent->state = PROCESS_BLOCKED;
    child->state = PROCESS_RUNNING;
    current_process = child;
    user_enter(entry, USER_STACK_TOP, cr3);
    return -1;
}