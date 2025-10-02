#include <asm_parser.h>

static bool isDirective(TokenType tt) {
  return tt == TokenType::ORG || tt == TokenType::DB;
}

static bool isMnemonic(TokenType tt) {
  uint16_t t = static_cast<uint16_t>(tt);

  return (0U <= t && t < 80U);
}

static optional<ast::Register> identToRegister(const string &ident) {
  if (ident.size() > 1)
    return {};

  // Verify if the ident is a valid register
  switch (ident[0]) {
  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'H':
  case 'L':
  case 'M':
    return static_cast<ast::Register>(ident[0]);
  default:
    break;
  }
  return {};
}

static optional<ast::ExtendedRegister> identToSpRegister(const string &ident) {
  if (ident.size() == 1) {
    switch (ident[0]) {
    case 'B':
      return ast::ExtendedRegister::B;
    case 'D':
      return ast::ExtendedRegister::D;
    case 'H':
      return ast::ExtendedRegister::H;
    }
  }
  if (ident == "PSW")
    return ast::ExtendedRegister::PSW;
  if (ident == "SP")
    return ast::ExtendedRegister::SP;
  return {};
}

Parser::Parser(vector<Token> &tokens)
    : m_tokens(move(tokens)), m_program(std::make_unique<ASTProgram>()) {}

ast::Ptr<ASTProgram> &Parser::parseProgram() {
  while (peek().has_value()) {
    if (peek().value().type == TokenType::EndOfFile)
      break;
    parseLine();
  }
  return m_program;
}

unordered_map<string, ast::symbolDebugInfo> &Parser::getSymbolTable() {
  m_symbolTable.rehash(m_symbolTable.size());
  return m_symbolTable;
}

// Private: Parsing Functions
void Parser::parseLine() {
  Token currToken = peek().value();

  if (isMnemonic(currToken.type)) {
    m_program->statements.emplace_back(parseMnemonic());

  } else if (isDirective(currToken.type)) {
    m_program->statements.emplace_back(parseDirective());

  } else if (currToken.type == TokenType::Identifier ||
             currToken.type == TokenType::HexOrIdent) {

    if (peek(1).has_value() && peek(1)->type == TokenType::Colon)
      m_program->statements.emplace_back(parseLabelDef());
    else {
      Logger::fmtLog(
          LogLevel::Error,
          "Unknown mnemonic or invalid statement '%s' on line %d, column %d",
          currToken.rawText.c_str(), currToken.line, currToken.column);
      exit(1);
    }
  }

  if (peek().has_value() && peek().value().type == TokenType::EndOfLine) {
    consume();
  } else {
    Logger::fmtLog(LogLevel::Error,
                   "Expected a EOL character on line: %d, a single line can "
                   "only have 1 instruction!",
                   currToken.line);
    exit(1);
  }
}

ast::Ptr<ASTLabelDef> Parser::parseLabelDef() {
  ast::Ptr<ASTLabelDef> labelDef = std::make_unique<ASTLabelDef>();
  Token label = consume();

  if (m_symbolTable.find(label.rawText) != m_symbolTable.end()) {
    Logger::fmtLog(LogLevel::Error,
                   "Redefinition of label '%s' on line: %d, column: %d",
                   label.rawText.c_str(), label.line, label.column);
    exit(1);
  }

  labelDef->tokenLabel = label;
  labelDef->labelDbgInfo = {.lineNumber = label.line,
                            .address = 0x0000,
                            .flag = 1,
                            .blockOffset = 0x0000};

  if (peek().has_value() && peek().value().type == TokenType::Colon)
    consume();
  else {
    Logger::fmtLog(LogLevel::Error,
                   "Expected a ':' after label '%s', on line: %d, column = %d",
                   label.rawText.c_str(), label.line, label.column);
    exit(1);
  }

  if (peek().has_value() && isMnemonic(peek().value().type))
    labelDef->mnemonic = parseMnemonic();
  else {
    Logger::fmtLog(
        LogLevel::Error,
        "Expected a instruction after label '%s', on line: %d, column = %d",
        label.rawText.c_str(), label.line, peek(-1).value().column);
    exit(1);
  }

  // On successful parsing, add labelDef to symbol table
  m_symbolTable[label.rawText] = labelDef->labelDbgInfo;
  return labelDef;
}

ast::Ptr<ASTMnemonics> Parser::parseMnemonic() {
  ast::Ptr<ASTMnemonics> mnemonic = std::make_unique<ASTMnemonics>();
  Token opcode = consume();
  mnemonic->instruction = opcode.type;
  mnemonic->tokenMnemonic = opcode;

  auto getExRegType = [&]() {
    return std::get<ast::Ptr<ASTExtendedRegister>>(
               mnemonic->operandList->first->val)
        ->exReg;
  };

  auto getImmValue = [&]() {
    return std::get<ast::Ptr<ASTImmData>>(mnemonic->operandList->first->val)
        ->value;
  };

  switch (mnemonic->instruction) {
  // Parse 'instruction <addr16>'
  case ast::InstructionType::LDA:
  case ast::InstructionType::LHLD:
  case ast::InstructionType::SHLD:
  case ast::InstructionType::STA:
    mnemonic->operandList = parseOpList({ast::OperandType::ImmAddr});
    break;

  // Parse 'instruction <imm8>'
  case ast::InstructionType::ACI:
  case ast::InstructionType::ADI:
  case ast::InstructionType::ANI:
  case ast::InstructionType::CPI:
  case ast::InstructionType::IN:
  case ast::InstructionType::ORI:
  case ast::InstructionType::OUT:
  case ast::InstructionType::SBI:
  case ast::InstructionType::SUI:
  case ast::InstructionType::XRI:
    mnemonic->operandList = parseOpList({ast::OperandType::ImmData});
    break;

  // Parse 'RST [0..7]'
  case ast::InstructionType::RST: {
    mnemonic->operandList = parseOpList({ast::OperandType::ImmData});
    uint8_t val = getImmValue();
    if (val < 0 || val > 7) {
      auto &token = mnemonic->tokenMnemonic;
      Logger::fmtLog(LogLevel::Error,
                     "Invalid value: '%d' for the instruction: 'RST' on "
                     "line: %d, column: %d",
                     val, token.line, token.column);
      exit(1);
    }
  } break;

  // Parse 'instruction <ex_reg>' ,
  // i.e, register pair + SP register (but not psw)
  case ast::InstructionType::DAD:
  case ast::InstructionType::DCX:
  case ast::InstructionType::INX:
  case ast::InstructionType::LXI: {
    // Main logic of the opcode
    mnemonic->operandList = parseOpList({ast::OperandType::exRegister});
    // Error handling for invalid operand type PSW
    if (getExRegType() == ast::ExtendedRegister::PSW) {
      auto &token = mnemonic->tokenMnemonic;
      Logger::fmtLog(LogLevel::Error,
                     "Invalid Operand: 'PSW' for the instruction: '%s'"
                     " on line: %d, column: %d",
                     token.rawText.c_str(), token.line, token.column);
      exit(1);
    }
  } break;

  // Parse LDAX ('B' | 'D') & STAX ('B' | 'D')
  case ast::InstructionType::LDAX:
  case ast::InstructionType::STAX: {
    mnemonic->operandList = parseOpList({ast::OperandType::exRegister});
    auto type = getExRegType();

    if (type != ast::ExtendedRegister::B && type != ast::ExtendedRegister::D) {
      auto &token = mnemonic->tokenMnemonic;
      Logger::fmtLog(LogLevel::Error,
                     "Expected Register Pair: 'B' OR 'D' for the instruction: "
                     "'%s' on line: "
                     "%d, column: %d",
                     token.rawText.c_str(), token.line, token.column);
      exit(1);
    }
  } break;
  case ast::InstructionType::POP:
  case ast::InstructionType::PUSH: {
    mnemonic->operandList = parseOpList({ast::OperandType::exRegister});
    auto type = getExRegType();

    if (type == ast::ExtendedRegister::SP) {
      auto &token = mnemonic->tokenMnemonic;
      Logger::fmtLog(LogLevel::Error,
                     "Invalid Operand: 'SP' for the instruction: '%s' on line: "
                     "%d, column: %d",
                     token.rawText.c_str(), token.line, token.column);
      exit(1);
    }
  } break;

  // Parse 'instruction <reg>'
  case ast::InstructionType::ADC:
  case ast::InstructionType::ADD:
  case ast::InstructionType::ANA:
  case ast::InstructionType::CMP:
  case ast::InstructionType::DCR:
  case ast::InstructionType::INR:
  case ast::InstructionType::ORA:
  case ast::InstructionType::SBB:
  case ast::InstructionType::SUB:
  case ast::InstructionType::XRA:
    mnemonic->operandList = parseOpList({ast::OperandType::_Register});
    break;

  // Parse 'instruction <labelRef>'
  case ast::InstructionType::CALL:
  case ast::InstructionType::CC:
  case ast::InstructionType::CM:
  case ast::InstructionType::CNC:
  case ast::InstructionType::CNZ:
  case ast::InstructionType::CP:
  case ast::InstructionType::CPE:
  case ast::InstructionType::CPO:
  case ast::InstructionType::CZ:
  case ast::InstructionType::JC:
  case ast::InstructionType::JM:
  case ast::InstructionType::JMP:
  case ast::InstructionType::JNC:
  case ast::InstructionType::JNZ:
  case ast::InstructionType::JP:
  case ast::InstructionType::JPE:
  case ast::InstructionType::JPO:
  case ast::InstructionType::JZ:
    mnemonic->operandList = parseOpList({ast::OperandType::LabelRef});
    break;

  // Parse double operand instructions MOV & MVI
  case ast::InstructionType::MOV:
    // MOV r, r | MOV M, r
    mnemonic->operandList =
        parseOpList({ast::OperandType::_Register, ast::OperandType::_Register});
    break;
  case ast::InstructionType::MVI:
    mnemonic->operandList =
        parseOpList({ast::OperandType::_Register, ast::OperandType::ImmData});
    break;
  default:
    // No instructions with no operand, don't need to be processed here
    mnemonic->operandList.reset();
    break;
  }

  return mnemonic;
}

ast::Ptr<ASTDirective> Parser::parseDirective() {
  ast::Ptr<ASTDirective> directive = std::make_unique<ASTDirective>();
  Token token = consume();
  directive->tokenDirective = token;

  if (token.type == TokenType::ORG) {
    directive->type = ast::DirectiveType::ORG;
    ast::Ptr<ASTImmAddr> addr = std::make_unique<ASTImmAddr>();

    if (peek().has_value() && (peek().value().type == TokenType::Number ||
                               peek().value().type == TokenType::HexOrIdent)) {
      addr->tokenAddr = peek().value();
      addr->value = parseNumber<uint16_t>();
    } else {
      Logger::fmtLog(LogLevel::Error,
                     "Expected a address after '%s' on line: %d, column: %d",
                     token.rawText.c_str(), token.line, token.column);
      exit(1);
    }

    directive->param = move(addr);
  } else if (token.type == TokenType::DB) {
    directive->type = ast::DirectiveType::DB;
    ast::Ptr<ASTImmData> data = std::make_unique<ASTImmData>();

    if (peek().has_value() && (peek().value().type == TokenType::Number ||
                               peek().value().type == TokenType::HexOrIdent)) {
      data->tokenData = peek().value();
      data->value = parseNumber<uint8_t>();
    } else {
      Logger::fmtLog(LogLevel::Error,
                     "Expected number after '%s' on line: %d, column: %d",
                     token.rawText.c_str(), token.line, token.column);
      exit(1);
    }

    directive->param = move(data);
  }

  return directive;
}

ast::Ptr<ASTOperandList>
Parser::parseOpList(const vector<ast::OperandType> &expectTypes) {
  ast::Ptr<ASTOperandList> operandList = std::make_unique<ASTOperandList>();

  int size = expectTypes.size();
  // size is always >= 1

  // Expected tokens ahead, as callee wont check if tokens are available
  // Parse operand will consume tokens
  if (peek().has_value())
    operandList->first = parseOperand(expectTypes[0]);
  else {
    Logger::fmtLog(LogLevel::Error,
                   "Expected a first operand, instead found '%s' at line: "
                   "%d, column: %d",
                   peek(-1).value().rawText.c_str(), peek(-1).value().line,
                   peek(-1).value().column);
    exit(1);
  }

  if (size == 1)
    return operandList;

  // Expect a comma token before the 2nd operand
  if (peek().has_value() && peek().value().type == TokenType::Comma)
    consume();
  else {
    Logger::fmtLog(LogLevel::Error,
                   "Expected a comma ',' after '%s' at line: %d, column: %d",
                   peek(-1).value().rawText.c_str(), peek(-1).value().line,
                   peek(-1).value().column);
    exit(1);
  }

  // else parse second operand
  if (peek().has_value())
    operandList->second = parseOperand(expectTypes[1]);
  else {
    Logger::fmtLog(LogLevel::Error,
                   "Expected a second operand, instead found '%s' at line: "
                   "%d, column: %d",
                   peek(-1).value().rawText.c_str(), peek(-1).value().line,
                   peek(-1).value().column);
    exit(1);
  }
  return operandList;
}

ast::Ptr<ASTOperand> Parser::parseOperand(const ast::OperandType &expectType) {
  // Assume the callee check if we can consume the token
  Token operandToken = peek().value();
  ast::Ptr<ASTOperand> astOperand = std::make_unique<ASTOperand>();

  switch (expectType) {
  case ast::OperandType::ImmData:
    if (operandToken.type == TokenType::Number ||
        operandToken.type == TokenType::HexOrIdent) {
      ast::Ptr<ASTImmData> immData = std::make_unique<ASTImmData>();
      immData->tokenData = operandToken;
      immData->value = parseNumber<uint8_t>();

      astOperand->val = move(immData);
    } else {
      Logger::fmtLog(
          LogLevel::Error,
          "Expected a number, but found '%s' on line: %d, column: %d",
          operandToken.rawText.c_str(), operandToken.line, operandToken.column);
      exit(1);
    }
    break;
  case ast::OperandType::ImmAddr:
    if (operandToken.type == TokenType::Number ||
        operandToken.type == TokenType::HexOrIdent) {
      ast::Ptr<ASTImmAddr> immAddr = std::make_unique<ASTImmAddr>();
      immAddr->tokenAddr = operandToken;
      immAddr->value = parseNumber<uint16_t>();

      astOperand->val = move(immAddr);
    } else {
      Logger::fmtLog(
          LogLevel::Error,
          "Expected a number, but found '%s' on line: %d, column: %d",
          operandToken.rawText.c_str(), operandToken.line, operandToken.column);
      exit(1);
    }
    break;
  case ast::OperandType::_Register:
    if ((operandToken.type == TokenType::Identifier ||
         operandToken.type == TokenType::HexOrIdent) &&
        identToRegister(operandToken.rawText).has_value()) {
      ast::Ptr<ASTRegister> reg = std::make_unique<ASTRegister>();
      reg->tokenRegister = operandToken;
      reg->reg = identToRegister(operandToken.rawText).value();
      consume(); // Manually consume this token

      astOperand->val = move(reg);
    } else {
      Logger::fmtLog(
          LogLevel::Error,
          "Expected a register, but found '%s' on line: %d, column: %d",
          operandToken.rawText.c_str(), operandToken.line, operandToken.column);
      exit(1);
    }
    break;
  case ast::OperandType::exRegister:
    if ((operandToken.type == TokenType::Identifier ||
         operandToken.type == TokenType::HexOrIdent) &&
        identToSpRegister(operandToken.rawText).has_value()) {
      ast::Ptr<ASTExtendedRegister> spReg =
          std::make_unique<ASTExtendedRegister>();
      spReg->tokenSpRegister = operandToken;
      spReg->exReg = identToSpRegister(operandToken.rawText).value();
      consume(); // Manually consume this token

      astOperand->val = move(spReg);
    } else {
      Logger::fmtLog(
          LogLevel::Error,
          "Expected a register, but found '%s' on line: %d, column: %d",
          operandToken.rawText.c_str(), operandToken.line, operandToken.column);
      exit(1);
    }
    break;
  case ast::OperandType::LabelRef:
    if (peek().has_value() && (peek().value().type == TokenType::Identifier ||
                               peek().value().type == TokenType::HexOrIdent)) {
      ast::Ptr<ASTLabelRef> labelRef = std::make_unique<ASTLabelRef>();
      labelRef->label = operandToken;
      consume(); // Manually consume this token

      astOperand->val = move(labelRef);
    } else {
      Logger::fmtLog(LogLevel::Error,
                     "Expected a label, but found '%s' on line: %d, column: %d",
                     operandToken.rawText, operandToken.line,
                     operandToken.column);
      exit(1);
    }
    break;
  default:
    Logger::fmtLog(LogLevel::Error, "Unexpected operand type found!");
    exit(1);
    break;
  }

  return astOperand;
}

template <typename T> T Parser::parseNumber() {
  Token &numToken = consume();
  string &num = numToken.rawText;

  int numBits = sizeof(T) * 8;
  unsigned int maxValue = pow(2, numBits) - 1;

  if (toupper(num.back()) == 'H') {
    // Address in hex format
    num.pop_back();
    unsigned int value = stoi(num, nullptr, 16);
    if (value > maxValue) {
      Logger::fmtLog(LogLevel::Error,
                     "Invalid number '%s' at line %d, column %d: value must "
                     "fit within %d-bit range (0–%d).",
                     num, numToken.line, numToken.column, numBits, maxValue);
      exit(1);
    }
    return (T)value;
  }
  // Base 10
  unsigned value = stoi(num, nullptr, 10);

  if (value > maxValue) {
    Logger::fmtLog(LogLevel::Error,
                   "Invalid number '%s' at line %d, column %d: value must fit "
                   "within %d-bit range (0–%d).",
                   num, numToken.line, numToken.column, numBits, maxValue);
    exit(1);
  }
  return (T)value;
}

optional<Token> Parser::peek(int next) {
  // Defaults next = 0
  if (m_currentTokenIndex + next < m_tokens.size())
    return m_tokens[m_currentTokenIndex + next];
  return {};
}

Token &Parser::consume() { return m_tokens[m_currentTokenIndex++]; }