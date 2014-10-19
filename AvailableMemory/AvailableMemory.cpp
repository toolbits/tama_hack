#include "AvailableMemory.h"
#include <stdlib.h>

namespace segundo {
namespace Utilities {

int AvailableMemory(int resolution, int maximum, bool disableInterrupts) {

    if (resolution < 1) resolution = 1;
    if (maximum < 0) maximum = 0;

    int low = 0;
    int high = maximum + 1;

    if (disableInterrupts) __disable_irq();

    while (high - low > resolution) {
        int mid = (low + high) / 2;
        void* p = malloc(mid);
        if (p == NULL) {
            high = mid;
        } else {
            free(p);
            low = mid;
        }
    }

    if (disableInterrupts) __enable_irq();

    return low;
}

} // namespace Utilities
} // namespace segundo