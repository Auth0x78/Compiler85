#pragma once

#include <ASTStructs.h>
#include <asm_lexer.h>
#include <memory>
#include <variant>
#include <vector>

using namespace std;

class Parser {
public:
  Parser(vector<Token> &tokens);
  ast::Ptr<ASTProgram> &parseProgram();

  unordered_map<string, ast::symbolDebugInfo> &getSymbolTable();

private:
  unique_ptr<ASTProgram> m_program;
  vector<Token> m_tokens;
  size_t m_currentTokenIndex = 0;

  // Parsing functions
  void parseLine();

  ast::Ptr<ASTLabelDef> parseLabelDef();

  ast::Ptr<ASTMnemonics> parseMnemonic();

  ast::Ptr<ASTDirective> parseDirective();

  ast::Ptr<ASTOperandList>
  parseOpList(const vector<ast::OperandType> &expectTypes);

  ast::Ptr<ASTOperand> parseOperand(const ast::OperandType &expectType);

  template <typename T> T parseNumber();

  optional<Token> peek(int next = 0);
  Token &consume();

  // Symbol Table, maps label names to line numbers/addresses
  unordered_map<string, ast::symbolDebugInfo> m_symbolTable;
};