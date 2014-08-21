/**
 * @file
 *
 * @date Aug 21, 2014
 * @author: Anton Bondarev
 */
#include <sys/mman.h>

#include <mem/vmem/vmem_alloc.h>
#include <mem/vmem.h>
#include <mem/mmap.h>

#include <mem/mapping/marea.h>

static const uint32_t mem_start = 0x04000000;
static const uint32_t mem_end = 0xFFFFF000;

extern void mmap_add_marea(struct emmap *mmap, struct marea *marea);
uint32_t mmap_userspace_create(struct emmap *emmap, size_t stack_size) {
	struct marea *marea;
	void *phy_addr;

	marea = marea_create(mem_end - stack_size, mem_end, PROT_READ | PROT_WRITE | PROT_EXEC);
	mmap_add_marea(emmap, marea);

	phy_addr = vmem_alloc_page();

	vmem_map_region(emmap->ctx, (mmu_paddr_t)phy_addr, mem_end - stack_size, stack_size, VMEM_PAGE_WRITABLE | VMEM_PAGE_USERMODE);

	return (uint32_t)phy_addr;
}
