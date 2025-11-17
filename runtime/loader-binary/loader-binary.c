#include "loader/loader.h"
#include "mm/vm.h"
#include "mm/mm.h"
#include "mm/common.h"
#include "mm/freemem.h"
#include "util/printf.h"
#include <asm/csr.h>

/* root page table */
pte root_page_table_storage[BIT(RISCV_PT_INDEX_BITS)] __attribute__((aligned(RISCV_PAGE_SIZE)));
/* page tables for loading physical memory */
pte load_l2_page_table_storage[BIT(RISCV_PT_INDEX_BITS)] __attribute__((aligned(RISCV_PAGE_SIZE)));
pte load_l3_page_table_storage[BIT(RISCV_PT_INDEX_BITS)] __attribute__((aligned(RISCV_PAGE_SIZE)));

uintptr_t free_base_final = 0;

uintptr_t satp_new(uintptr_t pa)
{
  return (SATP_MODE | (pa >> RISCV_PAGE_BITS));
}

void map_physical_memory(uintptr_t dram_base, uintptr_t dram_size) {
  uintptr_t ptr = EYRIE_LOAD_START;
  /* load address should not override kernel address */
  assert(RISCV_GET_PT_INDEX(ptr, 1) != RISCV_GET_PT_INDEX(RUNTIME_VA_START, 1));
  map_with_reserved_page_table(dram_base, dram_size,
      ptr, load_l2_page_table_storage, load_l3_page_table_storage);
}

int map_untrusted_memory(uintptr_t untrusted_ptr, uintptr_t untrusted_size) {
  uintptr_t va        = EYRIE_UNTRUSTED_START;
  while (va < EYRIE_UNTRUSTED_START + untrusted_size) {
    if (!map_page(vpn(va), ppn(untrusted_ptr), PTE_W | PTE_R | PTE_D)) {
      return -1;
    }
    va += RISCV_PAGE_SIZE;
    untrusted_ptr += RISCV_PAGE_SIZE;
  }
  return 0;
}

int map_shared_memory(uintptr_t sem_ptr, uintptr_t sem_size) {
  uintptr_t va        = EYRIE_SHARED_START;
  while (va < EYRIE_SHARED_START + sem_size) {
    if (!map_page(vpn(va), ppn(sem_ptr), PTE_W | PTE_R | PTE_D)) {  // access and valid are set in map_page
      return -1;
    }
    va += RISCV_PAGE_SIZE;
    sem_ptr += RISCV_PAGE_SIZE;
  }
  return 0;
}

int load_runtime(uintptr_t dummy,
                uintptr_t dram_base, uintptr_t dram_size, 
                uintptr_t runtime_base, uintptr_t user_base, 
                uintptr_t free_base, uintptr_t untrusted_ptr, 
                uintptr_t untrusted_size) {
  int ret = 0;

  // Extract shared enclave memory (SEM) parameters from registers passed by SM
  // These are stored in t3, t4, s9, s10, s11 by the Security Monitor
  uintptr_t sem_base = 0;         // t3: SEM physical address
  uintptr_t sem_size = 0;         // t4: SEM size in bytes
  uintptr_t sem_connector_base = 0; // s9: SEM connector physical address
  uintptr_t sem_connector_size = 0; // s10: SEM connector size in bytes
  uintptr_t sem_connector_vaddr = 0; // s11: SEM connector virtual address, not used

  // Use inline assembly to read the register values
  asm volatile (
    "mv %0, t3\n\t"  // sem_base = t3
    "mv %1, t4\n\t"  // sem_size = t4
    "mv %2, s9\n\t"  // sem_connector_base = s9
    "mv %3, s10\n\t" // sem_connector_size = s10
    "mv %4, s11"     // sem_connector_vaddr = s11
    : "=r" (sem_base), "=r" (sem_size), "=r" (sem_connector_base), 
      "=r" (sem_connector_size), "=r" (sem_connector_vaddr)
    : /* no input operands */
  );
  printf("[loader] SEM base: 0x%lx, SEM size: 0x%lx, SEM connector base: 0x%lx, SEM connector size: 0x%lx, SEM connector vaddr: 0x%lx\n",
         sem_base, sem_size, sem_connector_base, sem_connector_size, sem_connector_vaddr);

  root_page_table = root_page_table_storage;

  // initialize freemem
  spa_init(free_base, dram_base + dram_size - free_base);

  // validate runtime elf 
  size_t runtime_size = user_base - runtime_base;
  if (((void*) runtime_base == NULL) || (runtime_size <= 0)) {
    return -1; 
  }

  // create runtime elf struct
  elf_t runtime_elf;
  ret = elf_newFile((void*) runtime_base, runtime_size, &runtime_elf);
  if (ret != 0) {
    return ret;
  }

  // map runtime memory
  ret = loadElf(&runtime_elf, 0);
  if (ret != 0) {
    return ret;
  }

  // map enclave physical memory, so that runtime will be able to access all memory
  map_physical_memory(dram_base, dram_size);

  // map untrusted memory
  ret = map_untrusted_memory(untrusted_ptr, untrusted_size);
  if (ret != 0) {
    return ret;
  }

  // map shared memory
  // Now we have sem_base and sem_size from the registers
  if (sem_size > 0 && sem_base != 0) {
    ret = map_shared_memory(sem_base, sem_size);
    if (ret != 0) {
      return ret;
    }
  }

  // map connected memory if exists
  if (sem_connector_size > 0 && sem_connector_base != 0) {
    ret = map_shared_memory(sem_connector_base, sem_connector_size);
    if (ret != 0) {
      return ret;
    }
  }

  free_base_final = dram_base + dram_size - spa_available() * RISCV_PAGE_SIZE;

  return ret;
}

void error_and_exit() {
  printf("[loader] FATAL: failed to load.\n");
  sbi_exit_enclave(-1);
}

