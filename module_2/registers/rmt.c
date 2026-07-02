#include "rmt.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/*
 * Один RMT-символ займає 32 біти:
 *
 * bits [14:0]   duration0
 * bit  [15]     level0
 * bits [30:16]  duration1
 * bit  [31]     level1
 */
static uint32_t rmt_pack_item(uint16_t dur0, uint8_t lvl0,
                              uint16_t dur1, uint8_t lvl1)
{
    uint32_t word = 0;

    word = ((uint32_t)(dur0 & 0x7FFF))
         | ((uint32_t)(lvl0 & 0x01) << 15)
         | ((uint32_t)(dur1 & 0x7FFF) << 16)
         | ((uint32_t)(lvl1 & 0x01) << 31);

    return word;
}

static void rmt_debug_pack_item(void)
{
    uint32_t word;

    word = rmt_pack_item(5, 1, 3, 0);
    printf("pack(5, 1, 3, 0) = 0x%08lX\n", (unsigned long)word);

    word = rmt_pack_item(10, 0, 20, 1);
    printf("pack(10, 0, 20, 1) = 0x%08lX\n", (unsigned long)word);
}

void rmt_raw_init(int gpio, uint32_t resolution_hz)
{
    (void)gpio;
    (void)resolution_hz;

    /*
     * Stage 1 debug only.
     */
    rmt_debug_pack_item();
}

void rmt_raw_send_items(const rmt_item_t *items, size_t count)
{
    if (items == NULL || count == 0) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        uint32_t word = rmt_pack_item(
            items[i].dur0,
            items[i].lvl0,
            items[i].dur1,
            items[i].lvl1
        );

        printf("item[%u]: dur0=%u lvl0=%u dur1=%u lvl1=%u word=0x%08lX\n",
               (unsigned)i,
               items[i].dur0,
               items[i].lvl0,
               items[i].dur1,
               items[i].lvl1,
               (unsigned long)word);
    }
}
