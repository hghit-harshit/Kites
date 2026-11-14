#pragma once

#include <QString>
#include <cstdint>

namespace Kites
{
enum class Base
{
    Hexadecimal,
    Binary,
    SignedDecimal,
    UnsignedDecimal,
    Float // only meaningful for FP registers; integer data falls back to unsigned decimal
};

// Formats the low `bitWidth` bits of `value` (1..64) in the requested base.
inline QString formatInteger(uint64_t value, int bitWidth, Base base)
{
    const uint64_t masked = (bitWidth >= 64) ? value : (value & ((uint64_t{1} << bitWidth) - 1));
    switch (base)
    {
    case Base::Hexadecimal:
        return QString("0x%1").arg(QString::number(masked, 16).toUpper());
    case Base::Binary:
        return QString("0b%1").arg(QString::number(masked, 2));
    case Base::SignedDecimal:
    {
        const int shift = 64 - bitWidth;
        const int64_t sext = static_cast<int64_t>(masked << shift) >> shift;
        return QString::number(sext);
    }
    case Base::UnsignedDecimal:
    case Base::Float:
        return QString::number(masked);
    }
    return QString();
}
} // namespace Kites
