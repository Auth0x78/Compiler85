#pragma once
#include "ASTStructs.h"

#define DEFAULT_BLOCK_SIZE 32
#define UNREACHABLE(msg)                                                       \
  {                                                                            \
    Logger::fmtLog(LogLevel::Error, "[GEN]: " msg);                            \
    exit(0x00BAD);                                                             \
  }

using namespace std;

struct BinaryBlock {
  uint16_t startAddr;
  vector<uint8_t> code;
  // Store the offsets to where the label was used in the code
  vector<pair<string, uint16_t>> unresolvedLabel;

  // Default constructor
  BinaryBlock() : startAddr(0), unresolvedLabel({}) {
    code.reserve(DEFAULT_BLOCK_SIZE);
  }

  // Disable copy
  BinaryBlock(const BinaryBlock &) = delete;
  BinaryBlock &operator=(const BinaryBlock &) = delete;

  // Only keep a move constructor
  BinaryBlock(BinaryBlock &&other) noexcept
      : startAddr(other.startAddr), code(move(other.code)),
        unresolvedLabel(move(other.unresolvedLabel)) {}

  BinaryBlock &operator=(BinaryBlock &&other) noexcept {
    startAddr = other.startAddr;
    code = move(other.code);
    unresolvedLabel = move(other.unresolvedLabel);
    return *this;
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

  vector<BinaryBlock> &GenerateBinary();

  void GenerateStatement(ASTStatement &stmt);
  void GenerateMnemonics(const ast::Ptr<ASTMnemonics> &mnemonic);

  BinaryBlock &GetCurrentBlock();
  BinaryBlock &CreateCodeBlock();

  long long blockIndex;
  unordered_map<string, ast::symbolDebugInfo> m_symbolTable;

private:
  vector<BinaryBlock> m_blocks;

  size_t memAddrOffset;
  const size_t memAddrMax;
  ast::Ptr<ASTProgram> m_program;
};