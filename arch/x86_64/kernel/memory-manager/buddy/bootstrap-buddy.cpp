#include "x86_64/memory-manager/buddy/bootstrap-buddy.hpp"
#include "x86_64/trace/strace.hpp"
#include <stl/cstdlib/stdio.h>

namespace firefly::kernel::mm::buddy {
uint64_t ram_lowest, ram_highest = 0x0;

static size_t limine_mmap(struct stivale2_mmap_entry *mmap_entries, int total_entries, size_t buddy_size);
void bootstrap_buddy(struct stivale2_struct_tag_memmap *phys_mmap) {
    /*
    *  High level walkthrough on the setup of the buddy allocator
    * 
    *   1. Get the abs min and max address of the mmap and return the highest address in the mmap
    *   2. Calculate the amount of memory the buddy structure requires
    *   3. Find an entry in the mmap that is large enough to host the buddy allocator
    *   4. Initialize the buddy allocator
    *   5. Lastly the used entries are allocated
    */
    auto total_mem = ram_diff(phys_mmap->memmap, phys_mmap->entries);
    auto memory_used = Buddy.estimate_memory_used(total_mem);
    Buddy = { reinterpret_cast<void *>(ram_highest - ram_lowest) };
    [[maybe_unused]]void *mmap = reinterpret_cast<void*>(limine_mmap(phys_mmap->memmap, phys_mmap->entries, memory_used));
    // Buddy.initialize(total_mem, (char*) mmap); //Causes GPF - investigate
}

static inline uint64_t ram_diff(struct stivale2_mmap_entry *mmap_entries, int total_entries) {
    ram_lowest = mmap_entries[0].base;
    ram_highest = mmap_entries[total_entries-1].base + mmap_entries[total_entries-1].length;
    printf("ram_low: %X | ram_high: %X | ram total: %X\n", ram_lowest, ram_highest, ram_highest - ram_lowest);
    return ram_highest;
}

static size_t limine_mmap(struct stivale2_mmap_entry *mmap_entries, int total_entries, size_t buddy_size) {
    size_t result = 0;

    for (int current_entry = 0; current_entry < total_entries; current_entry++)
    {
        struct stivale2_mmap_entry *entry = &mmap_entries[current_entry];
        if (entry->type != STIVALE2_MMAP_USABLE)
            continue;
        
        if ((result = (entry->base + entry->length)) >= buddy_size) {
            printf("buddy_size: %X | entry: %X\n", buddy_size, result);
            return result;
        }
    }

    trace::panic("Cannot find a large enough entry to host the buddy allocator");
    __builtin_unreachable(); //Avoids the "not all code paths return a value" compiler error
}

}  // namespace firefly::kernel::mm::buddy