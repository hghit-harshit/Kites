// Characterization test: runs every example program on every processor, step by step, and
// compares a per-step fingerprint of the architectural + pipeline state against a recorded
// golden file. Then undoes every step and redoes every step, fingerprinting each, so undo/redo
// behaviour is pinned too.
//
// Regenerate the golden file (only when a behaviour change is intended):
//   set KITES_GOLDEN_RECORD=1 and run kites_test --gtest_filter=GoldenTrace.*

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

constexpr const char *goldenPath = "golden/golden_trace.txt";
constexpr int maxSteps = 5000;
constexpr uint64_t dataStart = 0x10000000;
constexpr int dataBytes = 512;
constexpr int stackBytes = 256;

class Fnv
{
  public:
    void add(uint64_t value)
    {
        for (int i = 0; i < 8; ++i)
        {
            m_hash ^= (value >> (i * 8)) & 0xff;
            m_hash *= 1099511628211ull;
        }
    }
    void add(const std::string &text)
    {
        for (char c : text)
        {
            m_hash ^= static_cast<uint8_t>(c);
            m_hash *= 1099511628211ull;
        }
        add(text.size());
    }
    [[nodiscard]] uint64_t value() const
    {
        return m_hash;
    }

  private:
    uint64_t m_hash = 14695981039346656037ull;
};

void hashArchState(Kites::ProcessorBase &vm, Fnv &fnv)
{
    fnv.add(vm.program_counter_);
    fnv.add(vm.cycle_s_);
    fnv.add(vm.instructions_retired_);
    fnv.add(vm.stall_cycles_);
    fnv.add(vm.branch_mispredictions_);
    fnv.add(vm.stop_requested_.load());
    fnv.add(vm.output_status_);
    for (uint64_t v : vm.registers_.GetGprValues())
        fnv.add(v);
    for (uint64_t v : vm.registers_.GetFprValues())
        fnv.add(v);
    for (size_t csr : {0x001, 0x002, 0x003})
        fnv.add(vm.registers_.ReadCsr(csr));
    for (int i = 0; i < dataBytes; ++i)
        fnv.add(vm.memory_controller_.peekByte(dataStart + i));
    // Window around the current stack pointer (sp starts at 0, so guard against wrap-around).
    const uint64_t sp = vm.registers_.ReadGpr(2);
    if (sp >= stackBytes)
    {
        for (int i = -stackBytes; i < stackBytes; ++i)
            fnv.add(vm.memory_controller_.peekByte(sp + i));
    }
    for (const QString &wire : vm.active_wires_)
        fnv.add(wire.toStdString());
}

// Exposes the protected pipeline registers of a 5-stage processor for fingerprinting.
template <typename Vm> struct PipelineProbe : Vm
{
    void hashPipeline(Fnv &fnv) const
    {
        const auto &a = this->if_id_reg_;
        fnv.add(a.pc);
        fnv.add(a.instruction);

        const auto &b = this->id_ex_reg_;
        for (uint64_t v : {b.pc, uint64_t(b.instruction), b.reg1_data, b.reg2_data,
                           uint64_t(int64_t(b.imm)), b.freg1_data, b.freg2_data, b.freg3_data,
                           uint64_t(b.rs1), uint64_t(b.rs2), uint64_t(b.rd), uint64_t(b.frs1),
                           uint64_t(b.frs2), uint64_t(b.frs3), uint64_t(b.frd),
                           uint64_t(b.reg_write), uint64_t(b.freg_write), uint64_t(b.mem_to_reg),
                           uint64_t(b.mem_read), uint64_t(b.mem_write), uint64_t(b.branch),
                           uint64_t(b.alu_src), uint64_t(b.alu_op)})
            fnv.add(v);

        const auto &c = this->ex_mem_reg_;
        for (uint64_t v : {c.pc, uint64_t(c.instruction), c.alu_result, c.reg2_data,
                           uint64_t(c.rd), uint64_t(c.prev_rd), c.f_alu_result, c.freg2_data,
                           uint64_t(c.frd), uint64_t(c.prev_frd), uint64_t(c.branch_taken),
                           uint64_t(c.prev_branch_taken), c.branch_target_pc,
                           uint64_t(c.reg_write), uint64_t(c.prev_reg_write),
                           uint64_t(c.freg_write), uint64_t(c.prev_freg_write),
                           uint64_t(c.mem_to_reg), uint64_t(c.mem_read), uint64_t(c.mem_write),
                           uint64_t(c.prev_mem_read), uint64_t(c.prev_mem_write)})
            fnv.add(v);

        const auto &d = this->mem_wb_reg_;
        for (uint64_t v : {d.pc, uint64_t(d.instruction), d.memory_data, d.alu_result,
                           uint64_t(d.rd), d.prev_memory_data, uint64_t(d.prev_rd),
                           d.prev_alu_result, d.f_memory_data, d.f_alu_result, uint64_t(d.frd),
                           d.prev_f_memory_data, d.prev_f_alu_result, uint64_t(d.prev_frd),
                           uint64_t(d.reg_write), uint64_t(d.freg_write), uint64_t(d.mem_to_reg),
                           uint64_t(d.prev_mem_to_reg), uint64_t(d.prev_reg_write),
                           uint64_t(d.prev_freg_write)})
            fnv.add(v);
    }
};

struct RvssProbe : Kites::RVSSProcessor
{
    void hashPipeline(Fnv &) const
    {
    }
};

template <typename Probe, int stepLimit = maxSteps>
std::string traceProgram(const std::filesystem::path &file)
{
    Kites::setupVmStateDirectory();
    auto vm = std::make_unique<Probe>();
    Kites::AssembledProgram program = Kites::assemble(file.string());
    vm->LoadProgram(program);

    auto fingerprint = [&]() {
        Fnv fnv;
        hashArchState(*vm, fnv);
        vm->hashPipeline(fnv);
        return fnv.value();
    };

    std::ostringstream out;
    out << std::hex;
    out << "init " << fingerprint() << "\n";

    int steps = 0;
    while (steps < stepLimit && !vm->stop_requested_)
    {
        bool more = static_cast<Kites::ProcessorBase &>(*vm).StepAndNotify();
        ++steps;
        out << "step " << fingerprint() << "\n";
        if (!more)
            break;
    }
    for (int i = 0; i < steps; ++i)
    {
        vm->Undo();
        out << "undo " << fingerprint() << "\n";
    }
    for (int i = 0; i < steps; ++i)
    {
        vm->Redo();
        out << "redo " << fingerprint() << "\n";
    }
    return out.str();
}

std::vector<std::filesystem::path> examplePrograms()
{
    std::vector<std::filesystem::path> files;
    for (const auto &dir : {"../examples", "../examples/fd-extension"})
    {
        for (const auto &entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".s")
                files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

using TraceFn = std::string (*)(const std::filesystem::path &);

std::map<std::string, std::string> computeAllTraces()
{
    const std::vector<std::pair<std::string, TraceFn>> processors = {
        {"rvss", &traceProgram<RvssProbe, 600>}, // rvss dumps state to disk every step
        {"h_f", &traceProgram<PipelineProbe<Kites::RV5StageProcessorHF>>},
        {"h_nf", &traceProgram<PipelineProbe<Kites::RV5StageProcessorHNF>>},
        {"nh_f", &traceProgram<PipelineProbe<Kites::RV5StageProcessorNHF>>},
        {"nh_nf", &traceProgram<PipelineProbe<Kites::RV5StageProcessorNHNF>>},
    };
    std::map<std::string, std::string> traces;
    for (const auto &file : examplePrograms())
    {
        for (const auto &[name, fn] : processors)
        {
            const std::string key = name + " " + file.generic_string();
            const auto start = std::chrono::steady_clock::now();
            try
            {
                traces[key] = fn(file);
            }
            catch (const std::exception &e)
            {
                // An exception is part of the observed behaviour; pin its message.
                traces[key] = std::string("exception ") + e.what() + "\n";
            }
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count();
            std::cerr << "[golden] " << key << " " << ms << "ms\n";
        }
    }
    return traces;
}

std::map<std::string, std::string> readGolden()
{
    std::map<std::string, std::string> traces;
    std::ifstream in(goldenPath);
    std::string line;
    std::string key;
    while (std::getline(in, line))
    {
        if (line.rfind("== ", 0) == 0)
        {
            key = line.substr(3);
            traces[key];
        }
        else
        {
            traces[key] += line + "\n";
        }
    }
    return traces;
}

} // namespace

TEST(GoldenTrace, AllExamplesAllProcessors)
{
    const auto traces = computeAllTraces();

    if (std::getenv("KITES_GOLDEN_RECORD"))
    {
        std::filesystem::create_directories("golden");
        std::ofstream out(goldenPath);
        for (const auto &[key, trace] : traces)
            out << "== " << key << "\n" << trace;
        GTEST_SKIP() << "Recorded " << traces.size() << " traces to " << goldenPath;
    }

    const auto golden = readGolden();
    ASSERT_FALSE(golden.empty()) << "Missing " << goldenPath << "; record with KITES_GOLDEN_RECORD=1";
    EXPECT_EQ(golden.size(), traces.size());

    for (const auto &[key, trace] : traces)
    {
        auto it = golden.find(key);
        if (it == golden.end())
        {
            ADD_FAILURE() << "No golden trace for " << key;
            continue;
        }
        if (it->second == trace)
            continue;

        std::istringstream expected(it->second);
        std::istringstream actual(trace);
        std::string e;
        std::string a;
        int line = 0;
        while (true)
        {
            bool he = static_cast<bool>(std::getline(expected, e));
            bool ha = static_cast<bool>(std::getline(actual, a));
            if (!he && !ha)
                break;
            if (e != a || he != ha)
            {
                ADD_FAILURE() << key << ": first divergence at trace line " << line
                              << " expected '" << (he ? e : "<eof>") << "' got '"
                              << (ha ? a : "<eof>") << "'";
                break;
            }
            ++line;
        }
    }
}
