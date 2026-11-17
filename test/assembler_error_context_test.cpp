/**
 * @file assembler_error_context_test.cpp
 * @brief Parse errors carry the right line number and source text, for both
 *        in-memory (stream) and file input, without re-opening any file.
 */

#include <gtest/gtest.h>

// Standard headers come first so the private->public macro below only touches
// project headers.
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <QTemporaryDir>

// Gives access to Parser::errors_ (same technique as test/pseudo_inst_test.cpp).
#define private public
#include "assembler/lexer.h"
#include "assembler/parser.h"
#undef private

#include "assembler/assembler.h"
#include "utils/utils.h"

namespace
{

struct ErrorContext
{
    unsigned int line;
    std::string text;
};

std::vector<ErrorContext> errorContexts(const Kites::Parser &parser)
{
    std::vector<ErrorContext> out;
    for (const auto &error : parser.errors_.all_errors)
    {
        std::visit([&](const auto &e) { out.push_back({e.line_number, e.line_text}); }, error);
    }
    return out;
}

// Lexes and parses without code generation, so the recorded errors can be inspected.
std::vector<ErrorContext> parseErrorsOf(std::istream &input, const std::string &filename = "",
                                        std::vector<unsigned int> *parseErrorLines = nullptr)
{
    Kites::Lexer lexer(input);
    auto tokens = lexer.getTokenList();
    Kites::Parser parser(filename.empty() ? lexer.getFilename() : filename, tokens,
                         lexer.getSourceLines());
    parser.parse();
    if (parseErrorLines)
    {
        for (const auto &e : parser.getErrors())
        {
            parseErrorLines->push_back(e.line);
        }
    }
    return errorContexts(parser);
}

std::vector<ErrorContext> parseErrorsOf(const std::string &source,
                                        std::vector<unsigned int> *parseErrorLines = nullptr)
{
    std::istringstream stream(source);
    return parseErrorsOf(stream, "", parseErrorLines);
}

} // namespace

TEST(AssemblerErrorContext, StreamErrorReportsParseFailureNotFileError)
{
    std::istringstream stream("addi a0, a0, 99999\n");
    try
    {
        Kites::assemble(stream);
        FAIL() << "expected a parse failure";
    }
    catch (const std::ios_base::failure &e)
    {
        FAIL() << "error context tried to re-open a file: " << e.what();
    }
    catch (const std::runtime_error &e)
    {
        EXPECT_STREQ(e.what(), "Failed to parse stream");
    }
}

TEST(AssemblerErrorContext, ImmediateOutOfRange)
{
    std::vector<unsigned int> lines;
    const auto errors = parseErrorsOf("addi a0, x0, 1\n"
                                      "addi a1, x0, 2\n"
                                      "addi a2, x0, 5000\n"
                                      "addi a3, x0, 3\n"
                                      "addi a4, x0, 4\n",
                                      &lines);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0].line, 3u);
    EXPECT_EQ(errors[0].text, "addi a2, x0, 5000");
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], 3u);
}

TEST(AssemblerErrorContext, UndefinedLabelFromBackPatchPass)
{
    const auto errors = parseErrorsOf("addi a0, x0, 1\n"
                                      "beq a0, x0, nowhere\n"
                                      "addi a1, x0, 2\n");
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].line, 2u);
    EXPECT_EQ(errors[0].text, "beq a0, x0, nowhere");
}

TEST(AssemblerErrorContext, LastLineWithoutTrailingNewline)
{
    const auto errors = parseErrorsOf("addi a0, x0, 1\nslliw a1, a0, 40");
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0].line, 2u);
    EXPECT_EQ(errors[0].text, "slliw a1, a0, 40");
}

TEST(AssemblerErrorContext, EachErrorGetsItsOwnLine)
{
    const auto errors = parseErrorsOf("addi a0, x0, 9999\n"
                                      "addi a1, x0, 1\n"
                                      "sraiw a2, a1, 32\n");
    ASSERT_EQ(errors.size(), 2u);
    EXPECT_EQ(errors[0].line, 1u);
    EXPECT_EQ(errors[0].text, "addi a0, x0, 9999");
    EXPECT_EQ(errors[1].line, 3u);
    EXPECT_EQ(errors[1].text, "sraiw a2, a1, 32");
}

TEST(AssemblerErrorContext, FileInput)
{
    Kites::setupVmStateDirectory();
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const std::filesystem::path path =
        std::filesystem::path(dir.path().toStdString()) / "bad_program.s";
    {
        std::ofstream out(path);
        out << "addi a0, x0, 1\n"
               "addi a1, x0, 70000\n";
    }

    {
        std::ifstream in(path);
        const auto errors = parseErrorsOf(in, path.string());
        ASSERT_EQ(errors.size(), 1u);
        EXPECT_EQ(errors[0].line, 2u);
        EXPECT_EQ(errors[0].text, "addi a1, x0, 70000");
    }

    // The file-based assemble() overload must report the parse failure too.
    EXPECT_THROW(Kites::assemble(path.string()), std::runtime_error);
}
