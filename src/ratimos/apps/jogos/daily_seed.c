#include <time.h>

#include "daily_seed.h"

#define SECONDS_PER_DAY 86400

uint32_t ratimos_daily_index(void)
{
    time_t now = time(NULL);
    if (now < 0) {
        return 0;
    }
    return (uint32_t) (now / SECONDS_PER_DAY);
}

uint32_t ratimos_daily_seed(uint8_t stream)
{
    /* 32 = folga suficiente para os 5 jogos de hoje mais os modos extras do
     * termo (dueto/quarteto) sem que dois fluxos colidam no mesmo dia. */
    return ratimos_daily_index() * 32u + (uint32_t) stream;
}
