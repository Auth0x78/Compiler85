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
        return;
    }
    void operator()(const ast::Ptr<ASTMnemonics> &mnemonic) {
      if (!mnemonic)
        return;
    }
    void operator()(const ast::Ptr<ASTDirective> &directive) {
      if (!directive)
        return;

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

BinaryBlock &AsmGenerator::GetCurrentBlock() { return m_blocks[blockIndex]; }

BinaryBlock &AsmGenerator::CreateCodeBlock() {
  blockIndex++;
  return m_blocks.emplace_back();
}