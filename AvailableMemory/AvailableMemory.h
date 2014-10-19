/** @file
 * Return the memory available for a malloc call.
 */
#ifndef SEGUNDO_UTILITIES_AVAILABLEMEMORY_H
#define SEGUNDO_UTILITIES_AVAILABLEMEMORY_H

/**
 * Segundo Equipo
 */
namespace segundo {
/**
 * A collection of utilities
 */
namespace Utilities {

/** Return the memory available for a malloc call.
 * This is done by a binary search approach
 * calling malloc/free starting with a maximum.
 *
 * Example:
 * @code
 * #include <stdio.h>
 * #include "AvailableMemory.h"
 *
 * int main() {
 *
 *     printf("Available memory (bytes to nearest 256) : %d\n", AvailableMemory());
 *     printf("Available memory (exact bytes) : %d\n", AvailableMemory(1));
 *
 * }
 * @endcode
 * @param resolution Resolution in number of bytes,
 * 1 will return the exact value,
 * default will return the available memory to the nearest 256 bytes
 * @param maximum Maximum amount of memory to check, default is 32K (0x8000)
 * @param disableInterrupts Disable interrupts whilst checking, default is true
 * @return Available memory in bytes accurate to within resolution
 */
int AvailableMemory(int resolution = 256, int maximum = 0x8000, bool disableInterrupts = true);

} // namespace Utilities
} // namespace segundo

using namespace segundo::Utilities;

#endif // SEGUNDO_UTILITIES_AVAILABLEMEMORY_H