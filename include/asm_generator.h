#pragma once
#include "ASTStructs.h"

using namespace std;
constexpr int DEFAULT_BLOCK_SIZE = 32;

[[noreturn]] inline void Unreachable(int line, const char *fmt, ...) {
  char buffer[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // Match your exact macro’s format
  Logger::fmtLog(LogLevel::Error, "[GEN, Line: %d]: %s", line - 1, buffer);
  std::exit(0x00BAD);
}

struct BinaryBlock {
  uint16_t startAddr;
  vector<uint8_t> code;
  uint8_t overflow;

  // Default constructor
  BinaryBlock() : startAddr(0), overflow(0) {
    code.reserve(DEFAULT_BLOCK_SIZE);
  }

  // Disable copy
  BinaryBlock(const BinaryBlock &) = delete;
  BinaryBlock &operator=(const BinaryBlock &) = delete;

  // Only keep a move constructor
  BinaryBlock(BinaryBlock &&other) noexcept
      : startAddr(other.startAddr), code(move(other.code)),
        overflow(other.overflow) {}

  BinaryBlock &operator=(BinaryBlock &&other) noexcept {
    startAddr = other.startAddr;
    overflow = other.overflow;
    code = move(other.code);
    return *this;
  }

  uint8_t &operator[](size_t index) {
    if (index < code.size())
      return code[index];
    else
      return overflow;
  }

  // Helper function, returns the absolute address of the byte pushed
  uint16_t AppendByte(uint8_t byte) {
    code.push_back(byte);
    return startAddr + code.size() - 1;
  }

  uint16_t AppendData(const vector<uint8_t> &data) {
    code.insert(code.end(), data.begin(), data.end());
    return startAddr + code.size() - data.size();
  }
};

class AsmGenerator {
public:
  AsmGenerator(ast::Ptr<ASTProgram> &program,
               unordered_map<string, ast::symbolDebugInfo> &symbolTable);

  void Print(FILE *outFile, const char *inputFileName) const;

  void GenerateBinary();
  vector<BinaryBlock> &getCodeBlock();

  void GenerateStatement(ASTStatement &stmt);
  void GenerateMnemonics(const ast::Ptr<ASTMnemonics> &mnemonic);
  void GenerateImmOperands(const ast::Ptr<ASTOperand> &operand,
                           const ast::OperandType expectedType);

  int TryRegisterMap(const ast::Ptr<ASTOperand> &operand);
  int TryRegisterExMap(const ast::Ptr<ASTOperand> &operand);

  constexpr uint8_t registerMapping(const ast::Register reg);

  constexpr uint8_t registerExMapping(const ast::ExtendedRegister regEx);

  BinaryBlock &GetCurrentBlock();
  BinaryBlock &CreateCodeBlock();

  long long blockIndex;
  unordered_map<string, ast::symbolDebugInfo> m_symbolTable;

  // Store the block id, block offset to where the unresolved label is found
  unordered_map<string, pair<size_t, uint16_t>> m_unresolvedLabel;
  vector<BinaryBlock> m_blocks;
  ast::Ptr<ASTProgram> m_program;
};