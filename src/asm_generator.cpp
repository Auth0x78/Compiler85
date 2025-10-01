#include "asm_generator.h"

// default constructor
AsmGenerator::AsmGenerator(
    ast::Ptr<ASTProgram> &program,
    unordered_map<string, ast::symbolDebugInfo> &symbolTable)
    : blockIndex(-1), m_program(move(program)),
      m_symbolTable(move(symbolTable)), m_unresolvedLabel({}) {}

void AsmGenerator::Print(FILE *outFile, const char *inputFileName) const {
  fprintf(outFile, "; %s", inputFileName);

  for (const auto &block : m_blocks) {
    int start = block.startAddr;

    for (int i = 0; i < block.code.size(); ++i)
      fprintf(outFile, "%04XH: %02XH\n", start + i, block.code[i]);
    fprintf(outFile, "\n");
  }
}

void AsmGenerator::GenerateBinary() {
  for (auto &stmt : m_program->statements) {
    GenerateStatement(stmt);
  }

  if (m_unresolvedLabel.size() != 0) {
    for (const auto &[label, info] : m_unresolvedLabel) {
      Logger::fmtLog(
          "Label Reference: '%s' on line: %d, column: %d was never defined!",
          label, -1, -1);
    }
    exit(1);
  }
}

vector<BinaryBlock> &AsmGenerator::getCodeBlock() { return m_blocks; }

void AsmGenerator::GenerateStatement(ASTStatement &stmt) {
  if (m_blocks.size() == 0)
    CreateCodeBlock();

  struct StmtVisitor {
    AsmGenerator *gen;
    void operator()(const ast::Ptr<ASTLabelDef> &labelDef) {
      if (!labelDef)
        UNREACHABLE("Label Definition was null in code generation!");
      BinaryBlock &block = gen->GetCurrentBlock();
      // Update symbol table with current address
      auto &[line, addr, flag, blockOffset] =
          gen->m_symbolTable[labelDef->tokenLabel.rawText];
      flag = 2;
      blockOffset = block.code.size();
      // TODO: verify correctness
      addr = block.startAddr + blockOffset;

      // Resolve any unresolved references to this label
      auto it = gen->m_unresolvedLabel.find(labelDef->tokenLabel.rawText);
      if (it != gen->m_unresolvedLabel.end()) {

        const auto &[blockID, offset] = it->second;

        uint8_t low = addr & 0xff;
        uint8_t high = (addr >> 8) & 0xff;

        // TODO: Only for debugging check if the memory locations are unused
        // Remove these extra checks later
        if (gen->m_blocks[blockID][offset] = 0xff)
          gen->m_blocks[blockID][offset] = low;
        else
          UNREACHABLE(
              "Resolving unresolved label failed, block offset already used");
        if (gen->m_blocks[blockID][(size_t)offset + 1] = 0xff)
          gen->m_blocks[blockID][(size_t)offset + 1] = high;
        else
          UNREACHABLE("Resolving unresolved label failed, block offset + 1 "
                      "already used");

        gen->m_unresolvedLabel.erase(labelDef->tokenLabel.rawText);
      }

      // Now generate the mnemonic associated with the label
      if (!labelDef->mnemonic)
        UNREACHABLE("Mnemonic was null in label definition!");
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
  BinaryBlock &block = GetCurrentBlock();
  const char *errorMsg = nullptr;

  auto &firstOperand = mnemonic->operandList->first;
  auto &secondOperand = mnemonic->operandList->second;

  switch (mnemonic->instruction) {
    // No operand instructions
  case ast::InstructionType::CMA:
    block.AppendByte(0x2F);
    break;
  case ast::InstructionType::CMC:
    block.AppendByte(0x3F);
    break;
  case ast::InstructionType::DAA:
    block.AppendByte(0x27);
    break;
  case ast::InstructionType::DI:
    block.AppendByte(0xF3);
    break;
  case ast::InstructionType::EI:
    block.AppendByte(0xFB);
    break;
  case ast::InstructionType::HLT:
    block.AppendByte(0x76);
    break;
  case ast::InstructionType::NOP:
    block.AppendByte(0x00);
    break;
  case ast::InstructionType::PCHL:
    block.AppendByte(0xE9);
    break;
  case ast::InstructionType::RAL:
    block.AppendByte(0x17);
    break;
  case ast::InstructionType::RAR:
    block.AppendByte(0x1F);
    break;
  case ast::InstructionType::RC:
    block.AppendByte(0xD8);
    break;
  case ast::InstructionType::RET:
    block.AppendByte(0xC9);
    break;
  case ast::InstructionType::RIM:
    block.AppendByte(0x20);
    break;
  case ast::InstructionType::RLC:
    block.AppendByte(0x07);
    break;
  case ast::InstructionType::RM:
    block.AppendByte(0xF8);
    break;
  case ast::InstructionType::RNC:
    block.AppendByte(0xD0);
    break;
  case ast::InstructionType::RNZ:
    block.AppendByte(0xC0);
    break;
  case ast::InstructionType::RP:
    block.AppendByte(0xF0);
    break;
  case ast::InstructionType::RPE:
    block.AppendByte(0xE8);
    break;
  case ast::InstructionType::RPO:
    block.AppendByte(0xE0);
    break;
  case ast::InstructionType::RRC:
    block.AppendByte(0x0F);
    break;
  case ast::InstructionType::RZ:
    block.AppendByte(0xC8);
    break;
  case ast::InstructionType::SIM:
    block.AppendByte(0x30);
    break;
  case ast::InstructionType::SPHL:
    block.AppendByte(0xF9);
    break;
  case ast::InstructionType::STC:
    block.AppendByte(0x37);
    break;
  case ast::InstructionType::XCHG:
    block.AppendByte(0xEB);
    break;
  case ast::InstructionType::XTHL:
    block.AppendByte(0xE3);
    break;
    // Single operand instructions
  case ast::InstructionType::ACI:
    block.AppendByte(0xCE);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::ADC: {
    // Opcode = 1111 + 1 _ register code(3 bits)
    // Extract register from operand list
    uint8_t baseOpcode = 0x88;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register operand for ADC instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::ADD: {
    uint8_t baseOpcode = 0x80;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register operand for ADD instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::ADI:
    block.AppendByte(0xC6);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::ANA: {
    uint8_t baseOpcode = 0xA0;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register operand for ANA instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::ANI:
    block.AppendByte(0xE6);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::CALL:
    block.AppendByte(0xCD);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CC:
    block.AppendByte(0xDC);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CM:
    block.AppendByte(0xFC);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CMP: {
    uint8_t baseOpcode = 0xB8;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register operand for ANA instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::CNC:
    block.AppendByte(0xD4);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CNZ:
    block.AppendByte(0xC4);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CP:
    block.AppendByte(0xF4);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CPE:
    block.AppendByte(0xEC);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CPI:
    block.AppendByte(0xFE);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::CPO:
    block.AppendByte(0xE4);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::CZ:
    block.AppendByte(0xCC);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::DAD: {
    // TODO: DAD PSW might cause problem as PSW AND SP return same ExReg Mapping
    uint8_t baseOpcode = 0x09;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg =
          "Expected a register pair operand for DAD instruction on line: %d, "
          "column: %d";
  } break;
  case ast::InstructionType::DCR: {
    uint8_t baseOpcode = 0x05;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 3) | baseOpcode);
    else
      errorMsg = "Expected a register operand for DCR instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::DCX: {
    // TODO: Might have PSW problem like DAD
    uint8_t baseOpcode = 0x0B;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg =
          "Expected a register pair operand for DAD instruction on line: %d, "
          "column: %d";
  } break;
  case ast::InstructionType::IN:
    block.AppendByte(0xDB);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::INR: {
    uint8_t baseOpcode = 0x04;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 3) | baseOpcode);
    else
      errorMsg = "Expected a register operand for INR instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::INX: {
    // TODO: Same issue as DAD
    uint8_t baseOpcode = 0x03;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg =
          "Expected a register pair operand for INX instruction on line: %d, "
          "column: %d";
  } break;
  case ast::InstructionType::JC:
    block.AppendByte(0xDA);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JM:
    block.AppendByte(0xFA);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JMP:
    block.AppendByte(0xC3);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JNC:
    block.AppendByte(0xD2);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JNZ:
    block.AppendByte(0xC2);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JP:
    block.AppendByte(0xF2);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JPE:
    block.AppendByte(0xEA);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JPO:
    block.AppendByte(0xE2);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::JZ:
    block.AppendByte(0xCA);
    GenerateImmOperands(firstOperand, ast::OperandType::LabelRef);
    break;
  case ast::InstructionType::LDA:
    block.AppendByte(0x3A);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmAddr);
    break;
  case ast::InstructionType::LDAX: {
    uint8_t baseOpcode = 0x0A;
    int res = TryRegisterExMap(firstOperand);
    if (res == 0 || res == 1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg = "Expected a register pair 'B' or 'D' for LDAX "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::LHLD:
    block.AppendByte(0x2A);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmAddr);
    break;
  case ast::InstructionType::LXI: {
    uint8_t baseOpcode = 0x01;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg = "Expected a register pair for LXI "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::ORA: {
    uint8_t baseOpcode = 0xB0;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register for ORA "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::ORI:
    block.AppendByte(0xF6);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::OUT:
    block.AppendByte(0xD3);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::POP: {
    uint8_t baseOpcode = 0xC1;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg = "Expected a register pair for POP "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::PUSH: {
    uint8_t baseOpcode = 0xC5;
    int res = TryRegisterExMap(firstOperand);
    if (res != -1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg = "Expected a register pair for PUSH "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::RST: {
    if (holds_alternative<ast::Ptr<ASTImmData>>(firstOperand->val)) {
      uint8_t baseOpcode = 0xC7;
      const auto &data = std::get<ast::Ptr<ASTImmData>>(firstOperand->val);

      if (data->value >= 0 && data->value < 8)
        block.AppendByte((data->value << 3) | baseOpcode);
      else
        errorMsg = "Expected a number from 0-7 for RST "
                   "instruction on line: %d, "
                   "column: %d";
    } else {
      errorMsg = "Expected a number from 0-7 for RST "
                 "instruction on line: %d, "
                 "column: %d";
    }
  } break;
  case ast::InstructionType::SBB: {
    uint8_t baseOpcode = 0x98;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = errorMsg = "Expected a register for SBB "
                            "instruction on line: %d, "
                            "column: %d";
  } break;
  case ast::InstructionType::SBI:
    block.AppendByte(0xDE);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::SHLD:
    block.AppendByte(0x22);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmAddr);
    break;
  case ast::InstructionType::STA:
    block.AppendByte(0x32);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmAddr);
    break;
  case ast::InstructionType::STAX: {
    uint8_t baseOpcode = 0x02;
    int res = TryRegisterExMap(firstOperand);
    if (res == 0 || res == 1)
      block.AppendByte((res << 4) | baseOpcode);
    else
      errorMsg = "Expected a register pair 'B' or 'D' for STAX "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::SUB: {
    uint8_t baseOpcode = 0x90;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register for SUB "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::SUI:
    block.AppendByte(0xD6);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::XRA: {
    uint8_t baseOpcode = 0xA8;
    int res = TryRegisterMap(firstOperand);
    if (res != -1)
      block.AppendByte(baseOpcode | res);
    else
      errorMsg = "Expected a register for XRA "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::XRI:
    block.AppendByte(0xEE);
    GenerateImmOperands(firstOperand, ast::OperandType::ImmData);
    break;
  case ast::InstructionType::MOV: {
    uint8_t baseOpcode = 0x40;
    int dst = TryRegisterMap(firstOperand);
    int src = TryRegisterMap(secondOperand);
    if (dst != -1 && src != -1)
      block.AppendByte(baseOpcode | (dst << 3) | src);
    else
      errorMsg = "Expected a dst and src register for MOV "
                 "instruction on line: %d, "
                 "column: %d";
  } break;
  case ast::InstructionType::MVI: {
    uint8_t baseOpcode = 0x06;
    int dst = TryRegisterMap(firstOperand);

    if (dst != -1)
      block.AppendByte(baseOpcode | (dst << 3));
    else
      errorMsg = "Expected a dst register for MVI "
                 "instruction on line: %d, "
                 "column: %d";
    GenerateImmOperands(secondOperand, ast::OperandType::ImmData);
  } break;
  default:
    UNREACHABLE("Instruction '%s' NYI!",
                mnemonic->tokenMnemonic.rawText.c_str());
    break;
  }

  if (errorMsg) {
    Logger::fmtLog(LogLevel::Error, errorMsg, mnemonic->tokenMnemonic.line,
                   mnemonic->tokenMnemonic.column);
    exit(1);
  }
}

void AsmGenerator::GenerateImmOperands(const ast::Ptr<ASTOperand> &operand,
                                       const ast::OperandType expectedType) {
  struct ASTOperandVisitor {
    AsmGenerator *gen;
    const ast::OperandType expectedType;

    void operator()(const ast::Ptr<ASTLabelRef> &labelRef) {
      if (!labelRef)
        UNREACHABLE("Label Reference is null!");
      if (expectedType != ast::OperandType::LabelRef)
        UNREACHABLE(
            "Expected a label reference operand, but got something else!");

      const string &labelT = labelRef->label.rawText;
      if (gen->m_symbolTable.find(labelT) == gen->m_symbolTable.end())
        UNREACHABLE("Label '%s' not found in symbol table!", labelT.c_str());

      const auto &[line, absAddr, flag, blockOffset] =
          gen->m_symbolTable[labelT];
      // LOW ADDR then HIGH ADDR
      if (flag == 2) {
        uint8_t low = absAddr & 0xff;
        uint8_t high = (absAddr >> 8) & 0xff;
        gen->GetCurrentBlock().AppendData({low, high});
      } else if (flag == 1) {
        // Label referenced before its definition
        // So, add the current label to unresolved label list
        gen->m_unresolvedLabel[labelT] = {gen->blockIndex, blockOffset};

        // Reserve 2 bytes for the address
        gen->GetCurrentBlock().AppendData({0xff, 0xff});
      } else {
        // Cant reach here, but still for safety
        UNREACHABLE("Label was defined, but flag was set to 0!");
      }
    }

    void operator()(const ast::Ptr<ASTImmData> &immData) {
      if (!immData)
        UNREACHABLE("Immediate Data is null!");
      if (expectedType != ast::OperandType::ImmData)
        UNREACHABLE(
            "Expected a Immediate Data operand, but got something else!");
      gen->GetCurrentBlock().AppendByte(immData->value);
    }

    void operator()(const ast::Ptr<ASTImmAddr> &immAddr) {
      if (!immAddr)
        UNREACHABLE("Immediate Address is null!");
      if (expectedType != ast::OperandType::ImmAddr)
        UNREACHABLE(
            "Expected a Immediate Address operand, but got something else!");

      uint8_t low = immAddr->value & 0xff;
      uint8_t high = (immAddr->value >> 8) & 0xff;
      gen->GetCurrentBlock().AppendData({low, high});
    }

    void operator()(const ast::Ptr<ASTRegister> &immData) {
      UNREACHABLE("Invalid immediate operand: got register");
    }
    void operator()(const ast::Ptr<ASTExtendedRegister> &immData) {
      UNREACHABLE("Invalid immediate operand: got extended register");
    }
  };

  if (operand)
    std::visit(ASTOperandVisitor{.gen = this, .expectedType = expectedType},
               operand->val);
  else
    UNREACHABLE("Failed to generate operand, value was NULL!");
}

int AsmGenerator::TryRegisterMap(const ast::Ptr<ASTOperand> &operand) {
  if (operand && std::holds_alternative<ast::Ptr<ASTRegister>>(operand->val)) {
    // Get the register
    const ast::Register reg =
        std::get<ast::Ptr<ASTRegister>>(operand->val)->reg;

    return registerMapping(reg);
  }

  return -1;
}

int AsmGenerator::TryRegisterExMap(const ast::Ptr<ASTOperand> &operand) {
  if (operand &&
      std::holds_alternative<ast::Ptr<ASTExtendedRegister>>(operand->val)) {
    // Get the register
    const ast::ExtendedRegister regEx =
        std::get<ast::Ptr<ASTExtendedRegister>>(operand->val)->exReg;

    return registerExMapping(regEx);
  }

  return -1;
}

constexpr uint8_t AsmGenerator::registerMapping(const ast::Register reg) {
  switch (reg) {
  case ast::Register::B:
    return 0b0000;
  case ast::Register::C:
    return 0b0001;
  case ast::Register::D:
    return 0b0010;
  case ast::Register::E:
    return 0b0011;
  case ast::Register::H:
    return 0b0100;
  case ast::Register::L:
    return 0b0101;
  case ast::Register::M:
    return 0b0110;
  case ast::Register::A:
    return 0b0111;
  }

  return 0b1111; // Cant reach here
}

constexpr uint8_t
AsmGenerator::registerExMapping(const ast::ExtendedRegister regEx) {
  switch (regEx) {
  case ast::ExtendedRegister::B:
    return 0b00;
  case ast::ExtendedRegister::D:
    return 0b01;
  case ast::ExtendedRegister::H:
    return 0b10;
  case ast::ExtendedRegister::SP:
  case ast::ExtendedRegister::PSW:
    return 0b11;
  }

  return 0b1111;
}

BinaryBlock &AsmGenerator::GetCurrentBlock() { return m_blocks[blockIndex]; }

BinaryBlock &AsmGenerator::CreateCodeBlock() {
  blockIndex++;
  return m_blocks.emplace_back();
}