#include "heap_probe.h"

#include "lvgl.h"

size_t ratimos_heap_used(void)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    return m.total_size - m.free_size;
}

size_t ratimos_heap_total(void)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    return m.total_size;
}

size_t ratimos_heap_peak(void)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    return m.max_used;
}

uint8_t ratimos_heap_used_pct(void)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    if (m.total_size == 0) {
        return 0;
    }
    return (uint8_t) (((uint64_t) (m.total_size - m.free_size) * 100u) / m.total_size);
}
