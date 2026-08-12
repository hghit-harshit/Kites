/**
 * @file pseudo_formats.cpp
 * @brief
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "assembler/parser.h"
#include "common/instructions.h"
#include "config/config.h"
#include "utils/utils.h"
#include "processor/registers.h"

#include <string>
namespace Kites
{
bool Parser::parse_pseudo()
{
    if (currentToken().value == "la")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            std::string reg = reg_alias_to_name.at(peekToken(1).value);
            std::string label = peekToken(3).value;

            if (symbol_table_.find(label) != symbol_table_.end() && symbol_table_[label].isData)
            {
                uint64_t address =
                    symbol_table_[label].address; // relative to data section (e.g., 0,8,16,...)
                uint64_t data_section_start = vm_config::config.getDataSectionStart();
                uint64_t symbol_addr = data_section_start + address;
                uint64_t pc = instruction_index_ * 4;
                int64_t offset = static_cast<int64_t>(symbol_addr) - static_cast<int64_t>(pc);
                int32_t hi20 = (offset + 0x800) >> 12;
                int32_t lo12 = offset - (hi20 << 12);

                ICUnit auipc_instr;
                auipc_instr.setOpcode("auipc");
                auipc_instr.setRd(reg);
                auipc_instr.setRs1("");
                auipc_instr.setRs2("");
                auipc_instr.setImm(std::to_string(hi20));
                auipc_instr.setLineNumber(currentToken().line_number);

                // std::cout << "auipc " << reg << ", " << "0x" << std::hex << hi20 << std::dec <<
                // std::endl;

                ICUnit addi_instr;
                addi_instr.setOpcode("addi");
                addi_instr.setRd(reg);
                addi_instr.setRs1(reg);
                addi_instr.setRs2("");
                addi_instr.setImm(std::to_string(lo12));
                addi_instr.setLineNumber(currentToken().line_number);

                // std::cout << "addi " << reg << ", " << reg << ", " << lo12 << std::dec <<
                // std::endl;

                auipc_instr.setInstructionIndex(instruction_index_);
                intermediate_code_.emplace_back(auipc_instr, true);
                instruction_number_line_number_mapping_[instruction_index_] =
                    auipc_instr.getLineNumber();
                instruction_index_++;

                addi_instr.setInstructionIndex(instruction_index_);
                intermediate_code_.emplace_back(addi_instr, true);
                instruction_number_line_number_mapping_[instruction_index_] =
                    addi_instr.getLineNumber();
                instruction_index_++;
            }
            else
            {
                recordError(ParseError(currentToken().line_number, "Invalid label reference"));
                errors_.all_errors.emplace_back(errors::InvalidLabelRefError(
                    "Invalid label reference", "Expected: Label defined in .data section",
                    filename_, currentToken().line_number, currentToken().column_number,
                    GetLineFromFile(filename_, currentToken().line_number)));
            }
            skipCurrentLine();
            // instruction_index_+=2;
            return true;
        }
        return false;
    }

    // nop
    else if (currentToken().value == "nop")
    {
        if (peekToken(1).type == TokenType::EOF_ ||
            peekToken(1).line_number != currentToken().line_number)
        {
            ICUnit block;
            block.setOpcode(currentToken().value);
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setOpcode("addi");
            block.setRd("x0");
            block.setRs1("x0");
            // block.setRs2("x0");
            block.setImm("0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            nextToken();
            return true;
        }
        return false;
    }

    // li
    else if (currentToken().value == "li")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::NUM &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode(currentToken().value);
            uint64_t imm = std::stoull(peekToken(3).value,nullptr,0);
            std::string reg = reg_alias_to_name.at(peekToken(1).value);

            auto emitAddi = [&](const std::string &rd, int64_t imm)
            {
                ICUnit addiBlock;
                addiBlock.setLineNumber(currentToken().line_number);
                addiBlock.setInstructionIndex(instruction_index_);
                addiBlock.setOpcode("addi");
                addiBlock.setRd(rd);
                addiBlock.setRs1(rd);
                addiBlock.setImm(std::to_string(imm));
                intermediate_code_.emplace_back(addiBlock, true);
                instruction_number_line_number_mapping_[instruction_index_++] = addiBlock.getLineNumber();
            };

            auto emitLui = [&](const std::string &rd, int64_t top20)
            {
                ICUnit luiBlock;
                luiBlock.setLineNumber(currentToken().line_number);
                luiBlock.setInstructionIndex(instruction_index_);
                luiBlock.setOpcode("lui");
                luiBlock.setRd(rd);
                luiBlock.setImm(std::to_string(top20));
                intermediate_code_.emplace_back(luiBlock, true);
                instruction_number_line_number_mapping_[instruction_index_++] = luiBlock.getLineNumber();
            };

            auto emitSlli = [&](const std::string &rd, int shift)
            {
                ICUnit slliBlock;
                slliBlock.setLineNumber(currentToken().line_number);
                slliBlock.setInstructionIndex(instruction_index_);
                slliBlock.setOpcode("slli");
                slliBlock.setRd(rd);
                slliBlock.setRs1(rd);
                slliBlock.setImm(std::to_string(shift));
                intermediate_code_.emplace_back(slliBlock, true);
                instruction_number_line_number_mapping_[instruction_index_++] = slliBlock.getLineNumber();
            };

            auto emitLi32 = [&](const std::string &rd, uint64_t value)
            {
                int64_t signedValue = static_cast<int64_t>(value);
                if (-2048 <= signedValue && signedValue <= 2047)
                {
                    ICUnit block;
                    block.setLineNumber(currentToken().line_number);
                    block.setInstructionIndex(instruction_index_);  
                    block.setOpcode("addi");
                    block.setRd(rd);
                    block.setRs1("x0");
                    block.setImm(std::to_string(signedValue));
                    intermediate_code_.emplace_back(block, true);
                    instruction_number_line_number_mapping_[instruction_index_++] = block.getLineNumber();
                    return;
                }

                int64_t upper = (signedValue + 2048) >> 12;
                int64_t lower = signedValue - (upper << 12);
                emitLui(rd, upper);
                if (lower != 0)
                    emitAddi(rd, lower);
            };

            auto countTrailingZeros64 = [](uint64_t v) -> int
            {
                if (v == 0)
                    return 64;
                int count = 0;
                while ((v & 1) == 0)
                {
                    v >>= 1;
                    ++count;
                }
                return count;
            };

            auto fitsIn32Bits = [](uint64_t value) -> bool
            {
                int64_t signedValue = static_cast<int64_t>(value);
                // Truncate to 32 bits, then sign-extend back to 64.
                // If that round-trip reproduces the original value, it fits in a signed 32-bit range.
                return signedValue == static_cast<int64_t>(static_cast<int32_t>(signedValue));
            };

            if (0 <= imm && imm <= UINT64_MAX)
            {
                // RV64: repeatedly peel off a 12-bit signed chunk until the
                // remaining "upper" value fits lui's 20-bit field. Collect the
                // chunks low-to-high, then emit high-to-low (lui, then
                // slli+addi per remaining chunk).
                std::function<void(const std::string &, uint64_t)> emitLi =
                [&](const std::string &rd, uint64_t value)
                {
                    if (fitsIn32Bits(value))
                    {
                        emitLi32(rd, value);
                        return;
                    }

                    uint64_t lo12 = value & 0xFFF;
                    if (lo12 & 0x800)
                        lo12 -= 0x1000; // signExtend12

                    value -= lo12;

                    int shift = countTrailingZeros64(static_cast<uint64_t>(value));
                    value >>= shift;

                    emitLi(rd, value);
                    emitSlli(rd, shift);
                    if (lo12 != 0)
                        emitAddi(rd, lo12);
                };
                emitLi(reg, imm);
            }
            else
            {
                recordError(ParseError(currentToken().line_number, "Immediate value out of range"));
                errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError(
                    "Immediate value out of range", "Expected: -2^63 <= imm <= 2^63 - 1", filename_,
                    currentToken().line_number, currentToken().column_number,
                    GetLineFromFile(filename_, currentToken().line_number)));
            }
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // mv
    else if (currentToken().value == "mv")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("add");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg);
            block.setRs2("x0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // not
    else if (currentToken().value == "not")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("xori");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg);
            block.setImm("-1");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // ret
    else if (currentToken().value == "ret")
    {
        if (peekToken(1).type == TokenType::EOF_ ||
            peekToken(1).line_number != currentToken().line_number)
        {
            ICUnit block;
            block.setOpcode("jalr");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x0");
            block.setRs1("x1");
            block.setImm("0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            nextToken();
            return true;
        }
        return false;
    }

    // neg
    else if (currentToken().value == "neg")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("sub");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1("x0");
            block.setRs2(reg);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // negw
    else if (currentToken().value == "negw")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("subw");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1("x0");
            block.setRs2(reg);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // sext.w
    else if (currentToken().value == "sext.w")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("addiw");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg);
            block.setImm("0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // seqz
    else if (currentToken().value == "seqz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("sltiu");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg);
            block.setImm("1");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // snez
    else if (currentToken().value == "snez")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("sltu");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1("x0");
            block.setRs2(reg);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // sltz
    else if (currentToken().value == "sltz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("slt");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg);
            block.setRs2("x0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
    }
    // sgtz
    else if (currentToken().value == "sgtz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("slt");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRd(reg);
            reg = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1("x0");
            block.setRs2(reg);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // beqz
    else if (currentToken().value == "beqz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("beq");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setRs2("x0");
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // bnez
    else if (currentToken().value == "bnez")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bne");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setRs2("x0");
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // blez
    else if (currentToken().value == "blez")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bge");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1("x0");
            block.setRs2(reg);
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // bgez
    else if (currentToken().value == "bgez")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bge");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setRs2("x0");
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // bltz
    else if (currentToken().value == "bltz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("blt");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setRs2("x0");
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // bgtz
    else if (currentToken().value == "bgtz")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::LABEL_REF &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("blt");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg;
            reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1("x0");
            block.setRs2(reg);
            block.setLabel(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // bgt
    else if (currentToken().value == "bgt")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            peekToken(4).line_number == currentToken().line_number &&
            peekToken(4).type == TokenType::COMMA &&
            peekToken(5).line_number == currentToken().line_number &&
            peekToken(5).type == TokenType::LABEL_REF &&
            (peekToken(6).type == TokenType::EOF_ ||
             peekToken(6).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("blt");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg1, reg2;
            reg1 = reg_alias_to_name.at(peekToken(1).value);
            reg2 = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg2);
            block.setRs2(reg1);
            block.setLabel(peekToken(5).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }
        return false;
    }

    // ble
    else if (currentToken().value == "ble")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            peekToken(4).line_number == currentToken().line_number &&
            peekToken(4).type == TokenType::COMMA &&
            peekToken(5).line_number == currentToken().line_number &&
            peekToken(5).type == TokenType::LABEL_REF &&
            (peekToken(6).type == TokenType::EOF_ ||
             peekToken(6).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bge");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg1, reg2;
            reg1 = reg_alias_to_name.at(peekToken(1).value);
            reg2 = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg2);
            block.setRs2(reg1);
            block.setLabel(peekToken(5).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // bgtu
    else if (currentToken().value == "bgtu")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            peekToken(4).line_number == currentToken().line_number &&
            peekToken(4).type == TokenType::COMMA &&
            peekToken(5).line_number == currentToken().line_number &&
            peekToken(5).type == TokenType::LABEL_REF &&
            (peekToken(6).type == TokenType::EOF_ ||
             peekToken(6).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bltu");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg1, reg2;
            reg1 = reg_alias_to_name.at(peekToken(1).value);
            reg2 = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg2);
            block.setRs2(reg1);
            block.setLabel(peekToken(5).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // bleu
    else if (currentToken().value == "bleu")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::GP_REGISTER &&
            peekToken(4).line_number == currentToken().line_number &&
            peekToken(4).type == TokenType::COMMA &&
            peekToken(5).line_number == currentToken().line_number &&
            peekToken(5).type == TokenType::LABEL_REF &&
            (peekToken(6).type == TokenType::EOF_ ||
             peekToken(6).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("bgeu");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            std::string reg1, reg2;
            reg1 = reg_alias_to_name.at(peekToken(1).value);
            reg2 = reg_alias_to_name.at(peekToken(3).value);
            block.setRs1(reg2);
            block.setRs2(reg1);
            block.setLabel(peekToken(5).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // j
    else if (currentToken().value == "j")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::LABEL_REF &&
            (peekToken(2).type == TokenType::EOF_ ||
             peekToken(2).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("jal");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x0");
            if (symbol_table_.find(peekToken(1).value) != symbol_table_.end() &&
                !symbol_table_[peekToken(1).value].isData)
            {
                uint64_t address = symbol_table_[peekToken(1).value].address;
                auto offset = static_cast<int64_t>(address - instruction_index_ * 4);
                if (-1048576 <= offset && offset <= 1048575)
                {
                    block.setImm(std::to_string(offset));
                    block.setLabel(peekToken(1).value);
                }
                else
                {
                    recordError(
                        ParseError(peekToken(1).line_number, "Immediate value out of range"));
                    errors_.all_errors.emplace_back(errors::ImmediateOutOfRangeError(
                        "Immediate value out of range", "Expected: -1048576 <= imm <= 1048575",
                        filename_, peekToken(1).line_number, peekToken(1).column_number,
                        GetLineFromFile(filename_, peekToken(1).line_number)));
                    skipCurrentLine();
                    return true;
                }
            }
            else
            {
                back_patch_.push_back(instruction_index_);
                block.setLabel(peekToken(1).value);
                intermediate_code_.emplace_back(block, false);
                instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
                instruction_index_++;
                skipCurrentLine();
                return true;
            }
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // jal // this might need looking into
    else if (currentToken().value == "jal")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::LABEL_REF &&
            (peekToken(2).type == TokenType::EOF_ ||
             peekToken(2).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("jal");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x1");
            block.setLabel(peekToken(1).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // jr
    else if (currentToken().value == "jr")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            (peekToken(2).type == TokenType::EOF_ ||
             peekToken(2).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("jalr");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x0");
            std::string reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setImm("0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // jalr
    else if (currentToken().value == "jalr")
    {
        if (peekToken(1).line_number == currentToken().line_number &&
            peekToken(1).type == TokenType::GP_REGISTER &&
            peekToken(2).line_number == currentToken().line_number &&
            peekToken(2).type == TokenType::COMMA &&
            peekToken(3).line_number == currentToken().line_number &&
            peekToken(3).type == TokenType::NUM &&
            (peekToken(4).type == TokenType::EOF_ ||
             peekToken(4).line_number != currentToken().line_number))
        {
            ICUnit block;
            block.setOpcode("jalr");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x1");
            std::string reg = reg_alias_to_name.at(peekToken(1).value);
            block.setRs1(reg);
            block.setImm(peekToken(3).value);
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    }

    // ret
    else if (currentToken().value == "ret")
    {
        if (peekToken(1).type == TokenType::EOF_ ||
            peekToken(1).line_number != currentToken().line_number)
        {
            ICUnit block;
            block.setOpcode("jalr");
            block.setLineNumber(currentToken().line_number);
            block.setInstructionIndex(instruction_index_);
            block.setRd("x0");
            block.setRs1("x1");
            block.setImm("0");
            intermediate_code_.emplace_back(block, true);
            instruction_number_line_number_mapping_[instruction_index_] = block.getLineNumber();
            instruction_index_++;
            skipCurrentLine();
            return true;
        }

        return false;
    } // call
    return false;
}
}// namespace Kites
