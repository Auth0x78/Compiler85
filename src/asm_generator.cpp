#include "asm_generator.h"

// default constructor
AsmGenerator::AsmGenerator(
    ast::Ptr<ASTProgram> &program,
    unordered_map<string, ast::symbolDebugInfo> &symbolTable)
    : blockIndex(-1), memAddrOffset(0), memAddrMax(0xFFFF),
      m_program(move(program)), m_symbolTable(move(symbolTable)) {}

vector<BinaryBlock> &AsmGenerator::GenerateBinary() {
  for (auto &stmt : m_program->statements) {
    GenerateStatement(stmt);
  }

  return m_blocks;
}

void AsmGenerator::GenerateStatement(ASTStatement &stmt) {
  if (m_blocks.size() == 0)
    CreateCodeBlock();

  struct StmtVisitor {
    AsmGenerator *gen;
    void operator()(const ast::Ptr<ASTLabelDef> &labelDef) {
      if (!labelDef)
        UNREACHABLE("Label Defination was null in code generation!");
      BinaryBlock &block = gen->GetCurrentBlock();
      // Update symbol table with current address
      auto &[line, addr, flag, blockOffset] =
          gen->m_symbolTable[labelDef->tokenLabel.rawText];
      flag = 2;
      blockOffset = block.code.size();
      addr = block.startAddr + addr;

      // Now generate the mnemonic associated with the label
      gen->GenerateMnemonics(labelDef->mnemonic);
    }
    void operator()(const ast::Ptr<ASTMnemonics> &mnemonic) {
      if (!mnemonic)
        UNREACHABLE("Mnemonic was null in code generation!");
      gen->GenerateMnemonics(mnemonic);
    }
    void operator()(const ast::Ptr<ASTDirective> &directive) {
      if (!directive)
        UNREACHABLE("Directive was null in code generation!");

      switch (directive->type) {
      case ast::DirectiveType::ORG: {
        BinaryBlock *block = nullptr;
        // If current block unused, use this block itself
        if (gen->GetCurrentBlock().code.size() == 0)
          block = &gen->GetCurrentBlock();
        else
          block = &gen->CreateCodeBlock();

        block->startAddr =
            std::get<ast::Ptr<ASTImmAddr>>(directive->param)->value;
      } break;
      case ast::DirectiveType::DB: {
        BinaryBlock &currentBlock = gen->GetCurrentBlock();
        uint8_t data = std::get<ast::Ptr<ASTImmData>>(directive->param)->value;
        currentBlock.AppendByte(data);
      } break;
      default:
        break;
      }
    }
  };

  std::visit(StmtVisitor{this}, stmt.sval);
}

void AsmGenerator::GenerateMnemonics(const ast::Ptr<ASTMnemonics> &mnemonic) {
  if (!mnemonic)
    UNREACHABLE("Mnemonic was null in code generation!");
  switch (mnemonic->instruction) {
  default:
    UNREACHABLE("Instruction '%s' NYI!",
                mnemonic->tokenMnemonic.rawText.c_str());
    break;
  }
}

BinaryBlock &AsmGenerator::GetCurrentBlock() { return m_blocks[blockIndex]; }

BinaryBlock &AsmGenerator::CreateCodeBlock() {
  blockIndex++;
  return m_blocks.emplace_back();
}