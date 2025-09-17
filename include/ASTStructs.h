#pragma once
#include <asm_lexer.h>
#include <memory>
#include <variant>
#include <vector>

namespace ast {
template <typename T> using Ptr = std::unique_ptr<T>;

using InstuctionType = TokenType;
using DirectiveType = TokenType;

// Symbol debug info struct
struct symbolDebugInfo {
  int lineNumber = 0;
  uint16_t address = 0; // will be filled in generator stage
  // default flag = 0 (uninitialized state), 1 (line number init), 2 (Rest Both)
  char flag = 0;
  size_t blockOffset = 0; // will be used in symbol resolution
};

enum class Register : char {
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  H = 'H',
  L = 'L',
  M = 'M'
};

enum class ExtendedRegister : char { B, D, H, SP, PSW };

enum class OperandType {
  ImmData,
  ImmAddr,
  LabelRef,
  _Register,
  exRegister,
};
}; // namespace ast

// AST structs
struct ASTLabelRef {
  Token label;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type and label
    printf("[ASTLabelRef]: %s\n", label.rawText.c_str());
  }
};

struct ASTExtendedRegister {
  ast::ExtendedRegister exReg;

  // Token for debugging info
  Token tokenSpRegister;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type and label
    printf("[ASTExtendedRegister]: %s\n", tokenSpRegister.rawText.c_str());
  }
};

struct ASTRegister {
  ast::Register reg;

  // Token for debugging
  Token tokenRegister;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type and label
    printf("[ASTRegister]: %s\n", tokenRegister.rawText.c_str());
  }
};

struct ASTImmAddr {
  uint16_t value;

  // Token for debug info
  Token tokenAddr;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type and label
    printf("[ASTImmAddr]: 0x%04X\n", value);
  }
};

struct ASTImmData {
  uint8_t value;

  // Token for debug info
  Token tokenData;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type and label
    printf("[ASTImmData]: 0x%02X\n", value);
  }
};

struct ASTOperand {
  variant<ast::Ptr<ASTLabelRef>, ast::Ptr<ASTImmData>, ast::Ptr<ASTImmAddr>,
          ast::Ptr<ASTRegister>, ast::Ptr<ASTExtendedRegister>>
      val;

  void Print(int id, int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTOperand] ID(%d):\n", id);

    // Print child operand
    std::visit(
        [h](auto &&nodePtr) {
          if (nodePtr) {
            nodePtr->Print(h + 1); // increase indentation for child
          }
        },
        val);
  }
};

struct ASTOperandList {
  // Optional params
  ast::Ptr<ASTOperand> first;

  ast::Ptr<ASTOperand> second;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTOperandList]:\n");
    // Print the child operands
    if (first)
      first->Print(1, h + 1);
    if (second)
      second->Print(2, h + 1);
  }
};

struct ASTDirective {
  ast::DirectiveType type;

  variant<ast::Ptr<ASTImmData>, ast::Ptr<ASTImmAddr>> param;

  // For debug info only
  Token tokenDirective;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTDirective] %s:\n", tokenDirective.rawText.c_str());

    // Print child paramaters
    std::visit(
        [h](auto &&nodePtr) {
          if (nodePtr) {
            nodePtr->Print(h + 1); // increase indentation for child
          }
        },
        param);
  }
};

struct ASTMnemonics {
  ast::InstuctionType instruction;

  ast::Ptr<ASTOperandList> operandList;

  // For debugging info, keep token
  Token tokenMnemonic;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTMnemonics] %s:\n", tokenMnemonic.rawText.c_str());

    // Print child operand list if avail
    if (operandList)
      operandList->Print(h + 1);
  }
};

struct ASTLabelDef {
  // Mnemonics must be present after label
  ast::Ptr<ASTMnemonics> mnemonic;

  // Store the actual name also
  Token tokenLabel;
  ast::symbolDebugInfo labelDbgInfo;

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTLabelDef] %s: {Line: %d, Address: 0x%04X}\n",
           tokenLabel.rawText.c_str(), labelDbgInfo.lineNumber,
           labelDbgInfo.address);
    // Print mnemonics associated with the label
    if (mnemonic)
      mnemonic->Print(h + 1);
  }
};

struct ASTStatement {
  variant<ast::Ptr<ASTMnemonics>, ast::Ptr<ASTLabelDef>, ast::Ptr<ASTDirective>>
      sval;

  // Constructor for ASTMnemonics
  ASTStatement(ast::Ptr<ASTMnemonics> m) : sval(std::move(m)) {}

  // Constructor for ASTLabelDef
  ASTStatement(ast::Ptr<ASTLabelDef> l) : sval(std::move(l)) {}

  // Constructor for ASTDirective
  ASTStatement(ast::Ptr<ASTDirective> d) : sval(std::move(d)) {}

  void Print(int h) {
    // Print indentation
    for (int i = 0; i < h; ++i)
      printf("  ");

    // Print node type
    printf("[ASTStatement]:\n");

    // Print child paramaters
    std::visit(
        [h](auto &&nodePtr) {
          if (nodePtr) {
            nodePtr->Print(h + 1); // increase indentation for child
          }
        },
        sval);
  }
};

struct ASTProgram {
  vector<ASTStatement> statements;

  void Print() {
    // Print ASTProgram header
    printf("The AST Tree contents are dumped below:\n");
    printf("[ASTProgram]:\n");

    for (auto &statement : statements)
      statement.Print(0 + 1);

    printf("\n");
  }
};