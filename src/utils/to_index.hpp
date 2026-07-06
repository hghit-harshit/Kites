#pragma once
#include <cstdint>
namespace Kites
{
    template <typename T>
    [[nodiscard]] constexpr std::size_t toIndex(T enumValue) noexcept
    {
        static_assert(std::is_enum_v<T>, "toIndex can only be used with enum types.");
        return static_cast<size_t>(static_cast<std::underlying_type_t<T>>(enumValue));
    }
}