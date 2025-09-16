#include <asm_parser.h>

static bool isDirective(TokenType tt) {
  return tt == TokenType::ORG || tt == TokenType::DB;
}

static bool isMnemonic(TokenType tt) {
  uint16_t t = static_cast<uint16_t>(tt);

  return (0 <= t && t < 79U);
}

static optional<ast::Register> identToRegister(const string &ident) {
  if (ident.size() > 1)
    return {};

  return static_cast<ast::Register>(ident[0]);
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

  if (currToken.type == TokenType::Identifier) {
    m_program->statements.emplace_back(parseLabelDef());
  } else if (isMnemonic(currToken.type)) {
    m_program->statements.emplace_back(parseMnemonic());
  } else if (isDirective(currToken.type)) {
    m_program->statements.emplace_back(parseDirective());
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

  labelDef->tokenLabel = label;
  labelDef->labelDbgInfo = {.lineNumber = label.line, .address = 0x0000};

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
  // TODO: Parse all mnemonics
  // Parse 'instruction <addr16>'
  case ast::InstuctionType::LDA:
  case ast::InstuctionType::LHLD:
  case ast::InstuctionType::SHLD:
  case ast::InstuctionType::STA:
    mnemonic->operandList = parseOpList({ast::OperandType::ImmAddr});
    break;

  // Parse 'instruction <imm8>'
  case ast::InstuctionType::ACI:
  case ast::InstuctionType::ADI:
  case ast::InstuctionType::ANI:
  case ast::InstuctionType::CPI:
  case ast::InstuctionType::IN:
  case ast::InstuctionType::ORI:
  case ast::InstuctionType::OUT:
  case ast::InstuctionType::SBI:
  case ast::InstuctionType::SUI:
  case ast::InstuctionType::XRI:
    mnemonic->operandList = parseOpList({ast::OperandType::ImmData});
    break;

  // Parse 'RST [0..7]'
  case ast::InstuctionType::RST: {
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
  case ast::InstuctionType::DAD:
  case ast::InstuctionType::DCX:
  case ast::InstuctionType::INX:
  case ast::InstuctionType::LXI: {
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
  case ast::InstuctionType::LDAX:
  case ast::InstuctionType::STAX: {
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
  case ast::InstuctionType::POP:
  case ast::InstuctionType::PUSH: {
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
  case ast::InstuctionType::ADC:
  case ast::InstuctionType::ADD:
  case ast::InstuctionType::ANA:
  case ast::InstuctionType::CMP:
  case ast::InstuctionType::DCR:
  case ast::InstuctionType::INR:
  case ast::InstuctionType::ORA:
  case ast::InstuctionType::SBB:
  case ast::InstuctionType::SUB:
  case ast::InstuctionType::XRA:
    mnemonic->operandList = parseOpList({ast::OperandType::_Register});
    break;

  // Parse 'instruction <labelRef>'
  case ast::InstuctionType::CALL:
  case ast::InstuctionType::CC:
  case ast::InstuctionType::CM:
  case ast::InstuctionType::CNC:
  case ast::InstuctionType::CNZ:
  case ast::InstuctionType::CP:
  case ast::InstuctionType::CPE:
  case ast::InstuctionType::CPO:
  case ast::InstuctionType::CZ:
  case ast::InstuctionType::JC:
  case ast::InstuctionType::JM:
  case ast::InstuctionType::JMP:
  case ast::InstuctionType::JNC:
  case ast::InstuctionType::JNZ:
  case ast::InstuctionType::JP:
  case ast::InstuctionType::JPE:
  case ast::InstuctionType::JPO:
  case ast::InstuctionType::JZ:
    mnemonic->operandList = parseOpList({ast::OperandType::LabelRef});
    break;

  // Parse double operand instructions MOV & MVI
  case ast::InstuctionType::MOV:
    // MOV r, r | MOV M, r
    mnemonic->operandList =
        parseOpList({ast::OperandType::_Register, ast::OperandType::_Register});
    break;
  case ast::InstuctionType::MVI:
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

    if (peek().has_value() && peek().value().type == TokenType::Number) {
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

    if (peek().has_value() && peek().value().type == TokenType::Number) {
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
    Logger::fmtLog(
        LogLevel::Error,
        "Expected a first operand, instead found '%s' at line: %d, column: %d",
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
    if (operandToken.type == TokenType::Number) {
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
    if (operandToken.type == TokenType::Number) {
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
    if (operandToken.type == TokenType::Identifier &&
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
    if (operandToken.type == TokenType::Identifier &&
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
    // TODO: Verify the Label isn't part of recognized words
    if (peek().has_value() && peek().value().type == TokenType::Identifier) {
      ast::Ptr<ASTLabelRef> labelRef = std::make_unique<ASTLabelRef>();
      labelRef->label = operandToken;
      consume(); // Manually consume this token

      astOperand->val = move(labelRef);
    } else {
      Logger::fmtLog(LogLevel::Error,
                     "Expected a label, but found '%s' on line: %d, column: %d",
                     operandToken);
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