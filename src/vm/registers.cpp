/**
 * @file registers.cpp
 * @brief Contains the implementation of the RegisterFile class and related
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#include "vm/registers.h"
#include "../mainwindow.h"
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <array>

RegisterFile::RegisterFile(QObject* parent) : QObject(parent)
{
  
}


void RegisterFile::Reset() {
  gpr_.fill(0);
  fpr_.fill(0.0);
  csr_.fill(0);
  csr_[0x002] = 0b000; // Default: RNE (IEEE 754)
}

uint64_t RegisterFile::ReadGpr(size_t reg) const {
  if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
  if (reg==0) return 0;
  return gpr_[reg];
}

void RegisterFile::WriteGpr(size_t reg, uint64_t value) {
  if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
  if (reg==0) return;
  //emit updateRegister(reg,value);
  
  gpr_[reg] = value;
}

uint64_t RegisterFile::ReadFpr(size_t reg) const {
  if (reg >= NUM_FPR) throw std::out_of_range("Invalid FPR index");
  return fpr_[reg];
}

void RegisterFile::WriteFpr(size_t reg, uint64_t value) {
  if (reg >= NUM_FPR) throw std::out_of_range("Invalid FPR index");
  fpr_[reg] = value;
}

uint64_t RegisterFile::ReadCsr(size_t reg) const {
  if (reg >= NUM_CSR) throw std::out_of_range("Invalid CSR index");
  return csr_[reg];
}

void RegisterFile::WriteCsr(size_t reg, uint64_t value) {
  if (reg >= NUM_CSR) throw std::out_of_range("Invalid CSR index");
  csr_[reg] = value;
}

std::vector<uint64_t> RegisterFile::GetGprValues() const {
  return {gpr_.begin(), gpr_.end()};
}

std::vector<uint64_t> RegisterFile::GetFprValues() const {
  return {fpr_.begin(), fpr_.end()};
}

void RegisterFile::ModifyRegister(const std::string &reg_name, uint64_t value) {
  std::string reg_name_n = reg_alias_to_name.at(reg_name);
  if (IsValidGeneralPurposeRegister(reg_name_n)) {
    WriteGpr(std::stoi(reg_name_n.substr(1)), value);
  } else if (IsValidFloatingPointRegister(reg_name_n)) {
    WriteFpr(std::stoi(reg_name_n.substr(1)), value);
  } else if (IsValidCsr(reg_name_n)) {
    WriteCsr(csr_to_address.at(reg_name_n), value);
  } else {
    throw std::invalid_argument("Invalid register name: " + reg_name_n);
  }
}

const std::unordered_set<std::string> valid_general_purpose_registers = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
    "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
    "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
    "x30", "x31",

    "zero",
    "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2",
    "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6",
};

const std::unordered_set<std::string> valid_floating_point_registers = {
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9",
    "f10", "f11", "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19",
    "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29",
    "f30", "f31",

    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
    "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
    "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
    "ft12", "ft13", "ft14", "ft15", "ft16", "ft17", "ft18", "ft19",
    "ft20", "ft21", "ft22", "ft23", "ft24", "ft25", "ft26", "ft27",
    "ft28", "ft29", "ft30", "ft31",
};

const std::unordered_set<std::string> valid_csr_registers = {
    "fflags", "frm", "fcsr"
};

const std::unordered_map<std::string, int> csr_to_address{
    {"fflags", 0x001},
    {"frm", 0x002},
    {"fcsr", 0x003},
};

const std::unordered_map<std::string, std::string> reg_alias_to_name = {
    {"zero", "x0"},
    {"ra", "x1"},
    {"sp", "x2"},
    {"gp", "x3"},
    {"tp", "x4"},
    {"t0", "x5"},
    {"t1", "x6"},
    {"t2", "x7"},
    {"s0", "x8"},
    {"fp", "x8"},
    {"s1", "x9"},
    {"a0", "x10"},
    {"a1", "x11"},
    {"a2", "x12"},
    {"a3", "x13"},
    {"a4", "x14"},
    {"a5", "x15"},
    {"a6", "x16"},
    {"a7", "x17"},
    {"s2", "x18"},
    {"s3", "x19"},
    {"s4", "x20"},
    {"s5", "x21"},
    {"s6", "x22"},
    {"s7", "x23"},
    {"s8", "x24"},
    {"s9", "x25"},
    {"s10", "x26"},
    {"s11", "x27"},
    {"t3", "x28"},
    {"t4", "x29"},
    {"t5", "x30"},
    {"t6", "x31"},

    {"x0", "x0"},
    {"x1", "x1"},
    {"x2", "x2"},
    {"x3", "x3"},
    {"x4", "x4"},
    {"x5", "x5"},
    {"x6", "x6"},
    {"x7", "x7"},
    {"x8", "x8"},
    {"x9", "x9"},
    {"x10", "x10"},
    {"x11", "x11"},
    {"x12", "x12"},
    {"x13", "x13"},
    {"x14", "x14"},
    {"x15", "x15"},
    {"x16", "x16"},
    {"x17", "x17"},
    {"x18", "x18"},
    {"x19", "x19"},
    {"x20", "x20"},
    {"x21", "x21"},
    {"x22", "x22"},
    {"x23", "x23"},
    {"x24", "x24"},
    {"x25", "x25"},
    {"x26", "x26"},
    {"x27", "x27"},
    {"x28", "x28"},
    {"x29", "x29"},
    {"x30", "x30"},
    {"x31", "x31"},

    {"ft0", "f0"},
    {"ft1", "f1"},
    {"ft2", "f2"},
    {"ft3", "f3"},
    {"ft4", "f4"},
    {"ft5", "f5"},
    {"ft6", "f6"},
    {"ft7", "f7"},
    {"fs0", "f8"},
    {"fs1", "f9"},
    {"fa0", "f10"},
    {"fa1", "f11"},
    {"fa2", "f12"},
    {"fa3", "f13"},
    {"fa4", "f14"},
    {"fa5", "f15"},
    {"fa6", "f16"},
    {"fa7", "f17"},
    {"fs2", "f18"},
    {"fs3", "f19"},
    {"fs4", "f20"},
    {"fs5", "f21"},
    {"fs6", "f22"},
    {"fs7", "f23"},
    {"fs8", "f24"},
    {"fs9", "f25"},
    {"fs10", "f26"},
    {"fs11", "f27"},
    {"ft8", "f28"},
    {"ft9", "f29"},
    {"ft10", "f30"},
    {"ft11", "f31"},

    {"f0", "f0"},
    {"f1", "f1"},
    {"f2", "f2"},
    {"f3", "f3"},
    {"f4", "f4"},
    {"f5", "f5"},
    {"f6", "f6"},
    {"f7", "f7"},
    {"f8", "f8"},
    {"f9", "f9"},
    {"f10", "f10"},
    {"f11", "f11"},
    {"f12", "f12"},
    {"f13", "f13"},
    {"f14", "f14"},
    {"f15", "f15"},
    {"f16", "f16"},
    {"f17", "f17"},
    {"f18", "f18"},
    {"f19", "f19"},
    {"f20", "f20"},
    {"f21", "f21"},
    {"f22", "f22"},
    {"f23", "f23"},
    {"f24", "f24"},
    {"f25", "f25"},
    {"f26", "f26"},
    {"f27", "f27"},
    {"f28", "f28"},
    {"f29", "f29"},
    {"f30", "f30"},
    {"f31", "f31"},

    {"fflags", "fflags"},
    {"frm", "frm"},
    {"fcsr", "fcsr"},

};

bool IsValidGeneralPurposeRegister(const std::string &reg) {
  return valid_general_purpose_registers.find(reg)!=valid_general_purpose_registers.end();
}

bool IsValidFloatingPointRegister(const std::string &reg) {
  return valid_floating_point_registers.find(reg)!=valid_floating_point_registers.end();
}

bool IsValidCsr(const std::string &reg) {
  return valid_csr_registers.find(reg)!=valid_csr_registers.end();
}
