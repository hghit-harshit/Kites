#pragma once
namespace Kites
{
namespace debug_color
{
/** @file debug_colors.h
 *  @brief Debug color definitions for console output.
 */
inline constexpr const char *red = "\033[31m";
inline constexpr const char *green = "\033[32m";
inline constexpr const char *yellow = "\033[33m";
inline constexpr const char *blue = "\033[34m";
inline constexpr const char *magenta = "\033[35m";
inline constexpr const char *cyan = "\033[36m";
inline constexpr const char *reset = "\033[0m";
}
}// namespace Kites