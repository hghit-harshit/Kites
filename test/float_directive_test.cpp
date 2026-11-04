/**
 * @file float_directive_test.cpp
 * @brief .float and .double data directives must store literals at full precision.
 */

#include <gtest/gtest.h>

#include <limits>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "assembler/assembler.h"

namespace
{

Kites::AssembledProgram assembleSource(const std::string &source)
{
    std::istringstream stream(source);
    return Kites::assemble(stream);
}

template <typename T>
std::vector<T> dataValuesOf(const Kites::AssembledProgram &program)
{
    std::vector<T> values;
    for (const auto &entry : program.data_buffer)
    {
        if (const auto *v = std::get_if<T>(&entry))
        {
            values.push_back(*v);
        }
    }
    return values;
}

} // namespace

TEST(FloatDirective, DoubleKeepsFullPrecision)
{
    auto program = assembleSource(".data\n"
                                  "a: .double 3.141592653589793\n"
                                  "b: .double 1e-7\n"
                                  "c: .double -2.5e10\n"
                                  "d: .double 2.2250738585072014e-308\n"
                                  ".text\naddi x0, x0, 0\n");
    const auto values = dataValuesOf<double>(program);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(values[0], 3.141592653589793);
    EXPECT_EQ(values[1], 1e-7);
    EXPECT_EQ(values[2], -2.5e10);
    EXPECT_EQ(values[3], std::numeric_limits<double>::min());
}

TEST(FloatDirective, FloatKeepsFullPrecision)
{
    auto program = assembleSource(".data\n"
                                  "a: .float 0.1\n"
                                  "b: .float 1e-7\n"
                                  "c: .float 3.4028235e38\n"
                                  ".text\naddi x0, x0, 0\n");
    const auto values = dataValuesOf<float>(program);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(values[0], 0.1f);
    EXPECT_EQ(values[1], 1e-7f);
    EXPECT_EQ(values[2], std::numeric_limits<float>::max());
}

TEST(FloatDirective, SeveralValuesOnOneLine)
{
    auto program = assembleSource(".data\n"
                                  "v: .double 1.5, -2.25, 0.123456789012345, 1e300\n"
                                  ".text\naddi x0, x0, 0\n");
    const auto values = dataValuesOf<double>(program);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(values[0], 1.5);
    EXPECT_EQ(values[1], -2.25);
    EXPECT_EQ(values[2], 0.123456789012345);
    EXPECT_EQ(values[3], 1e300);
}
