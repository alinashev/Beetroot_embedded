#include "rmt.h"

int main(void)
{
    rmt_raw_init(4, 1000000);

    rmt_item_t items[] = {
        {5,  1, 3,  0},
        {10, 0, 20, 1}
    };

    rmt_raw_send_items(items, 2);

    return 0;
}