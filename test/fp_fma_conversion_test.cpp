/**
 * @file fp_fma_conversion_test.cpp
 * @brief Fused multiply-add (fmadd/fmsub/fnmsub/fnmadd, .s and .d) and the
 *        precision/integer conversions fcvt.d.s, fcvt.s.d, fcvt.d.lu, fcvt.d.l
 *        on every processor model.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

const std::string kPad = "addi x0, x0, 0\naddi x0, x0, 0\naddi x0, x0, 0\n";

// Doubles first so every value sits at a fixed, naturally aligned offset.
const std::string kProgram = ".data\n"
                             "d: .double 2.0, 3.0, 0.5, 1.25\n" // 0, 8, 16, 24
                             "s: .float 2.0, 3.0, 0.5\n"        // 32, 36, 40
                             ".text\n"
                             "lui s0, 0x10000\n" +
                             kPad +
                             "fld f0, 0(s0)\n"
                             "fld f1, 8(s0)\n"
                             "fld f2, 16(s0)\n"
                             "fld f4, 24(s0)\n"
                             "flw f10, 32(s0)\n"
                             "flw f11, 36(s0)\n"
                             "flw f12, 40(s0)\n"
                             "addi t0, x0, -1\n" +
                             kPad +
                             "fmadd.d f5, f0, f1, f2\n"
                             "fmsub.d f6, f0, f1, f2\n"
                             "fnmsub.d f7, f0, f1, f2\n"
                             "fnmadd.d f8, f0, f1, f2\n"
                             "fmadd.s f13, f10, f11, f12\n"
                             "fmsub.s f14, f10, f11, f12\n"
                             "fnmsub.s f15, f10, f11, f12\n"
                             "fnmadd.s f16, f10, f11, f12\n"
                             "fcvt.d.s f17, f12\n"
                             "fcvt.s.d f18, f4\n"
                             "fcvt.d.lu f19, t0\n"
                             "fcvt.d.l f20, t0\n" +
                             kPad;

uint64_t doubleBits(double d)
{
    uint64_t bits;
    std::memcpy(&bits, &d, sizeof(bits));
    return bits;
}

uint32_t floatBits(float f)
{
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

template <typename Vm>
class FpFmaConversion : public ::testing::Test
{
  protected:
    static std::unique_ptr<Vm> run()
    {
        Kites::setupVmStateDirectory();
        auto vm = std::make_unique<Vm>();
        std::istringstream stream(kProgram);
        vm->LoadProgram(Kites::assemble(stream));
        vm->DebugRun();
        return vm;
    }
};

using AllVms = ::testing::Types<Kites::RVSSProcessor, Kites::RV5StageProcessorHF,
                                Kites::RV5StageProcessorHNF, Kites::RV5StageProcessorNHF,
                                Kites::RV5StageProcessorNHNF>;
TYPED_TEST_SUITE(FpFmaConversion, AllVms);

} // namespace

TYPED_TEST(FpFmaConversion, DoublePrecisionFma)
{
    auto vm = TestFixture::run();
    auto &r = vm->registers_;
    // a = 2, b = 3, c = 0.5
    EXPECT_EQ(r.ReadFpr(5), doubleBits(6.5)) << "fmadd.d  a*b + c";
    EXPECT_EQ(r.ReadFpr(6), doubleBits(5.5)) << "fmsub.d  a*b - c";
    EXPECT_EQ(r.ReadFpr(7), doubleBits(-5.5)) << "fnmsub.d -(a*b) + c";
    EXPECT_EQ(r.ReadFpr(8), doubleBits(-6.5)) << "fnmadd.d -(a*b) - c";
}

TYPED_TEST(FpFmaConversion, SinglePrecisionFma)
{
    auto vm = TestFixture::run();
    auto &r = vm->registers_;
    // Compare the single-precision bits in the low word.
    EXPECT_EQ(static_cast<uint32_t>(r.ReadFpr(13)), floatBits(6.5f)) << "fmadd.s";
    EXPECT_EQ(static_cast<uint32_t>(r.ReadFpr(14)), floatBits(5.5f)) << "fmsub.s";
    EXPECT_EQ(static_cast<uint32_t>(r.ReadFpr(15)), floatBits(-5.5f)) << "fnmsub.s";
    EXPECT_EQ(static_cast<uint32_t>(r.ReadFpr(16)), floatBits(-6.5f)) << "fnmadd.s";
}

TYPED_TEST(FpFmaConversion, PrecisionConversions)
{
    auto vm = TestFixture::run();
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadFpr(17), doubleBits(0.5)) << "fcvt.d.s";
    EXPECT_EQ(static_cast<uint32_t>(r.ReadFpr(18)), floatBits(1.25f)) << "fcvt.s.d";
}

TYPED_TEST(FpFmaConversion, IntegerToDoubleConversions)
{
    auto vm = TestFixture::run();
    auto &r = vm->registers_;
    EXPECT_EQ(r.ReadFpr(19), doubleBits(18446744073709551615.0)) << "fcvt.d.lu of 2^64-1";
    EXPECT_EQ(r.ReadFpr(20), doubleBits(-1.0)) << "fcvt.d.l of -1";
}
