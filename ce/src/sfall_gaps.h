#ifndef SFALL_GAPS_H
#define SFALL_GAPS_H

#include "interpreter.h"

namespace fallout {

// Notes a sfall opcode, metarule or hook a script asked for and CE lacks
// (task 186): the first time per script and map it prints a "sfall gap:"
// line, and sfallGapReport prints every gap with its count.
void sfallGapNote(const char* kind, const char* name, Program* program);
void sfallGapReport();

} // namespace fallout

#endif /* SFALL_GAPS_H */
