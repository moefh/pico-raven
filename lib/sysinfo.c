#include <malloc.h>

#include "sysinfo.h"

extern char __StackLimit, __bss_end__;

uint32_t sysinfo_heap_total(void)
{
    return &__StackLimit - &__bss_end__;
}

uint32_t sysinfo_heap_free(void)
{
   struct mallinfo m = mallinfo();

   return sysinfo_heap_total() - m.uordblks;
}
