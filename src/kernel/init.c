#include <stivale.h>
#include <sound/sound.h>
#include <mem/gdt.h>
#include <int/isr.h>
#include <int/idt.h>
#include <ps2/ps2.h>
#include <pit/pit.h>
#include <io/io.h>
#include <common.h>
#include <mem/mem.h>
//#include <liballoc.h>

uint8_t* mem = (uint8_t *)0;

void printmm() {
    kprintf("MEM MAP AT 0x%_x:\n", (uint32_t)(stivale_info->memory_map_addr), 8);
    struct stivale_mmap_entry* memmap = (struct stivale_mmap_entry*)(intptr_t)(stivale_info->memory_map_addr);
    for (size_t i = 0; i < stivale_info->memory_map_entries; i++) {
        struct stivale_mmap_entry e = memmap[i];
        kprintf("MEM %_x: [0x%_x - 0x%_x] (0x%_x) ", i, 2, (uint32_t)e.base, 8, (uint32_t)(e.base + e.length) - 1, 8, (unsigned int)e.type, 4);
        switch (e.type) {
            case STIVALE_MMAP_USABLE:
                usable_mem += e.length;
                free_mem += e.length;
                kprintf("[FREE: %u bytes]\n", e.length);
                break;
            case STIVALE_MMAP_RESERVED:
                usable_mem += e.length;
                kputs("[RESERVED]");
                break;
            case STIVALE_MMAP_ACPI_RECLAIMABLE:
                usable_mem += e.length;
                kputs("[ACPI RECLAIMABLE]");
                break;
            case STIVALE_MMAP_ACPI_NVS:
                kputs("[ACPI NVS]");
                break;
            case STIVALE_MMAP_BAD_MEMORY:
                kputs("[BAD]");
                break;
            case STIVALE_MMAP_KERNEL_AND_MODULES:
                usable_mem += e.length;
                kprintf("[KERNEL/MODULES: %u bytes]\n", e.length);
                break;
            case STIVALE_MMAP_BOOTLOADER_RECLAIMABLE:
                usable_mem += e.length;
                kputs("[BOOTLOADER RECLAIMABLE]");
                break;
            case STIVALE_MMAP_FRAMEBUFFER:
                usable_mem += e.length;
                kputs("[FRAMEBUFFER]");
                break;
            default:
                kputs("[UNKNOWN]");
                break;
        }
    }
}

void printfb() {
    kprintf("GFX FB AT 0x%_x:\n", (uint32_t)(stivale_info->framebuffer_addr), 8);
    kprintf(
        "GFX FB: [%u x %u] [%u BPP] [%u PITCH]\n",
        (uint32_t)(stivale_info->framebuffer_width),
        (uint32_t)(stivale_info->framebuffer_height),
        (uint32_t)(stivale_info->framebuffer_bpp),
        (uint32_t)(stivale_info->framebuffer_pitch)
    );
    kprintf("TXTBUF: [%u x %u]\n", textw, texth);
}

int _init_kpc_l = 0;

void _init_kputchar(char c) {
    outb(COM1, c);
    _std_kputchar(c);
    if (!curx && c != '\r') _init_kpc_l++;
    if (_init_kpc_l >= texth - 1) {
        _init_kpc_l = 0;
        uint64_t tmpticks = ticks;
        uint64_t tmpsyncticks = rtc_sync_val;
        anykey(false, true, NULL);
        rtc_sync_val = tmpsyncticks;
        ticks = tmpticks;
    }
}

void init() {
    _kputchar = _init_kputchar;
    kputs(logo);
    //_kputchar = _init_kputchar;
    vcursorstyle(14, 15);
    init_gdt();
    isr_install();
    irq_install();
    init_pit(1000);
    //init_ps2();
    init_keyboard();
    init_serial(COM1);
    serial_puts(COM1, "Opened COM1\n");
    printmm();
    printfb();
    if (!fpupresent) _kputs("No ");
    kputs("FPU found");
    /*
    kprintf("GFXPTR: 0x%_x\n", (unsigned int)gfxmode_ptr, 8);
    kprintf("GFXTXT: 0x%_x\n", (unsigned int)gfxmode_vbuf, 8);
    kprintf("GFXBAK: 0x%_x\n", (unsigned int)gfxmode_bakbuf, 8);
    */
    _kputchar = _std_kputchar;
    boot_ticks = ticks;
}
