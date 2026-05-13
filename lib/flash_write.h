#ifndef FLASH_WRITE_H_FILE
#define FLASH_WRITE_H_FILE

// - if using core 1, you MUST call flash_safe_execute_core_init() on
//   core 1 before calling flash_io_write().
// - flash_addr must be aligned to FLASH_PAGE_SIZE (4096).
// - size must be a multiplt of FLASH_PAGE_SIZE (4096).
int flash_write(uint32_t flash_offset, const void *data, uint32_t size);

#endif /* FLASH_WRITE_H_FILE */
