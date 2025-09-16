#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Logger.h>

using namespace std;

enum class TokenType : uint16_t {
  // Data Transfer
  MOV = 0,
  MVI,
  LXI,
  LDA,
  STA,
  LHLD,
  SHLD,
  LDAX,
  STAX,
  XCHG,

  // Arithmetic
  ADD,
  ADI,
  ADC,
  ACI,
  SUB,
  SUI,
  SBB,
  SBI,
  INR,
  DCR,
  INX,
  DCX,
  DAD,
  DAA,

  // Logical
  ANA,
  ANI,
  XRA,
  XRI,
  ORA,
  ORI,
  CMP,
  CPI,
  RLC,
  RRC,
  RAL,
  RAR,
  CMA,
  CMC,
  STC,

  // Branch
  JMP,
  JC,
  JNC,
  JZ,
  JNZ,
  JP,
  JM,
  JPE,
  JPO,
  CALL,
  CC,
  CNC,
  CZ,
  CNZ,
  CP,
  CM,
  CPE,
  CPO,
  RET,
  RC,
  RNC,
  RZ,
  RNZ,
  RP,
  RM,
  RPE,
  RPO,
  RST,
  PCHL,

  // Stack & Machine Control
  PUSH,
  POP,
  XTHL,
  SPHL,
  IN,
  OUT,
  HLT,
  NOP,
  DI,
  EI,
  RIM,
  SIM,

  // Assembler directives
  ORG,
  DB,

  // Non-instruction tokens
  Identifier, // labels
  Number,     // numeric constants
  Comma,
  Colon,
  EndOfLine,
  EndOfFile
};

struct Token {
  TokenType type;
  string rawText;
  int line;
  int column;
};

class Lexer {
public:
  Lexer(string &src);

  vector<Token> tokenize();

private:
  string m_source;
  size_t m_pos;
  int m_line;
  int m_col;

  optional<char> peek();
  char consume();

  Token createToken(TokenType ttype, const string &value);
};
