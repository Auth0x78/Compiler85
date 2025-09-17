#pragma once
#include "ASTStructs.h"

#define DEFAULT_BLOCK_SIZE 32

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

  // Helper function
  void AppendByte(uint8_t byte) { code.push_back(byte); }

  void AppendData(const vector<uint8_t> &data) {
    code.insert(code.end(), data.begin(), data.end());
  }
};

class AsmGenerator {
public:
  AsmGenerator(ast::Ptr<ASTProgram> &program,
               unordered_map<string, ast::symbolDebugInfo> &symbolTable);

  vector<BinaryBlock> &GenerateBinary();

  void GenerateStatement(ASTStatement &stmt);

  BinaryBlock &GetCurrentBlock();
  BinaryBlock &CreateCodeBlock();

  long long blockIndex;

private:
  vector<BinaryBlock> m_blocks;

  size_t memAddrOffset;
  const size_t memAddrMax;
  ast::Ptr<ASTProgram> m_program;

  // Assign each label a unique_id
  unordered_map<string, ast::symbolDebugInfo> m_symbolTable;
};