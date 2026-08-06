#pragma once
#include <QString>

namespace Kites
{

// A single assembler-reported problem, normalized from the assembler's
// various error struct types (see src/assembler/errors.h) into one shape
// the editor can render as a squiggle without knowing about assembler internals.
struct Diagnostic
{
    int line;   // 1-based, matches editor line numbers
    int column; // 1-based
    QString message;
};

} // namespace Kites
