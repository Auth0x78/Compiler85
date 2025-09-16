#include <asm_lexer.h>

const static unordered_map<string, TokenType> keywordToToken = {
    // Data Transfer
    {"MOV", TokenType::MOV},
    {"MVI", TokenType::MVI},
    {"LXI", TokenType::LXI},
    {"LDA", TokenType::LDA},
    {"STA", TokenType::STA},
    {"LHLD", TokenType::LHLD},
    {"SHLD", TokenType::SHLD},
    {"LDAX", TokenType::LDAX},
    {"STAX", TokenType::STAX},
    {"XCHG", TokenType::XCHG},

    // Arithmetic
    {"ADD", TokenType::ADD},
    {"ADI", TokenType::ADI},
    {"ADC", TokenType::ADC},
    {"ACI", TokenType::ACI},
    {"SUB", TokenType::SUB},
    {"SUI", TokenType::SUI},
    {"SBB", TokenType::SBB},
    {"SBI", TokenType::SBI},
    {"INR", TokenType::INR},
    {"DCR", TokenType::DCR},
    {"INX", TokenType::INX},
    {"DCX", TokenType::DCX},
    {"DAD", TokenType::DAD},
    {"DAA", TokenType::DAA},

    // Logical
    {"ANA", TokenType::ANA},
    {"ANI", TokenType::ANI},
    {"XRA", TokenType::XRA},
    {"XRI", TokenType::XRI},
    {"ORA", TokenType::ORA},
    {"ORI", TokenType::ORI},
    {"CMP", TokenType::CMP},
    {"CPI", TokenType::CPI},
    {"RLC", TokenType::RLC},
    {"RRC", TokenType::RRC},
    {"RAL", TokenType::RAL},
    {"RAR", TokenType::RAR},
    {"CMA", TokenType::CMA},
    {"CMC", TokenType::CMC},
    {"STC", TokenType::STC},

    // Branch
    {"JMP", TokenType::JMP},
    {"JC", TokenType::JC},
    {"JNC", TokenType::JNC},
    {"JZ", TokenType::JZ},
    {"JNZ", TokenType::JNZ},
    {"JP", TokenType::JP},
    {"JM", TokenType::JM},
    {"JPE", TokenType::JPE},
    {"JPO", TokenType::JPO},
    {"CALL", TokenType::CALL},
    {"CC", TokenType::CC},
    {"CNC", TokenType::CNC},
    {"CZ", TokenType::CZ},
    {"CNZ", TokenType::CNZ},
    {"CP", TokenType::CP},
    {"CM", TokenType::CM},
    {"CPE", TokenType::CPE},
    {"CPO", TokenType::CPO},
    {"RET", TokenType::RET},
    {"RC", TokenType::RC},
    {"RNC", TokenType::RNC},
    {"RZ", TokenType::RZ},
    {"RNZ", TokenType::RNZ},
    {"RP", TokenType::RP},
    {"RM", TokenType::RM},
    {"RPE", TokenType::RPE},
    {"RPO", TokenType::RPO},
    {"RST", TokenType::RST},
    {"PCHL", TokenType::PCHL},

    // Stack & Machine Control
    {"PUSH", TokenType::PUSH},
    {"POP", TokenType::POP},
    {"XTHL", TokenType::XTHL},
    {"SPHL", TokenType::SPHL},
    {"IN", TokenType::IN},
    {"OUT", TokenType::OUT},
    {"HLT", TokenType::HLT},
    {"NOP", TokenType::NOP},
    {"DI", TokenType::DI},
    {"EI", TokenType::EI},
    {"RIM", TokenType::RIM},
    {"SIM", TokenType::SIM},

    // Directives
    {"ORG", TokenType::ORG},
    {"DB", TokenType::DB}};

Lexer::Lexer(string &src)
    : m_source(std::move(src)), m_pos(0), m_line(1), m_col(0) {}

vector<Token> Lexer::tokenize() {
  vector<Token> tokens;

  while (peek().has_value()) {
    char curr = consume();
    if (isspace(curr)) {
      if (curr == '\n') {
        tokens.emplace_back(createToken(TokenType::EndOfLine, "\\n"));
        m_line++;
        m_col = 0;
      }
      continue;
    } else if (isalpha(curr)) {
      string curr_str(1, curr);
      while (peek().has_value() &&
             (isalpha(peek().value()) || peek().value() == '_'))
        curr_str += consume();

      if (keywordToToken.find(curr_str) != keywordToToken.end())
        tokens.emplace_back(createToken(keywordToToken.at(curr_str), curr_str));
      else
        tokens.emplace_back(createToken(TokenType::Identifier, curr_str));
    } else if (isdigit(curr)) {
      string num(1, curr);
      while (peek().has_value() &&
             (isdigit(peek().value()) || peek().value() == 'H' ||
              peek().value() == 'h'))
        num += consume();
      tokens.emplace_back(createToken(TokenType::Number, num));
    } else if (curr == ',') {
      tokens.emplace_back(createToken(TokenType::Comma, string(1, curr)));
    } else if (curr == ':') {
      tokens.emplace_back(createToken(TokenType::Colon, string(1, curr)));
    } else if (curr == ';') {
      // Start of comment, skip until end of line
      while (peek().has_value() && peek().value() != '\n')
        consume();
    } else {
      Logger::fmtLog(LogLevel::Error,
                     "Unexpected character '%c' at line %d, column %d", curr,
                     m_line + 1, m_col);
      return {}; // Empty vector
    }
  }

  // Emplace back eof
  tokens.emplace_back(createToken(TokenType::EndOfLine, ""));
  tokens.emplace_back(createToken(TokenType::EndOfFile, ""));
  return tokens;
}

optional<char> Lexer::peek() {
  if (m_pos < m_source.size()) {
    return m_source[m_pos];
  }
  return {};
}

char Lexer::consume() {
  m_col++;
  return m_source[m_pos++];
}

Token Lexer::createToken(TokenType ttype, const string &rawText) {
  return Token{ttype, rawText, m_line,
               m_col - static_cast<int>(rawText.size())};
}
