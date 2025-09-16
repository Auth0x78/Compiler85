// Compiler85.cpp : Defines the entry point for the application.
//
#include <Compiler85.h>
using namespace std;

int main(int argv, char *argc[]) {
  // Usage: c85 <sourceFile> <outputFile> <flag>?
  // flag: -r -> output file is raw binary, otherwise output is .hex format
  string sourceFile;
  string outputFile;
  bool rawBinary = false;

#ifdef DEBUG
  Logger::fmtLog("Debug mode: No command line arguments required.");
  Logger::fmtLog("Enter the filepath of the source file: ");
  cin >> sourceFile;
  Logger::fmtLog("Enter the filepath of the output file: ");
  cin >> outputFile;
  rawBinary = true;
#else
  if (argv < 3) {
    Logger::fmtLog(LogLevel::Info, "\n\tUsage: c85 <sourceFile> <outputFile>");
    return 1;
  }
  sourceFile = argc[1];
  outputFile = argc[2];
  if (argv > 3)
    rawBinary = (string(argc[3]) == "-r");
#endif // !DEBUG

  // Read source file
  string src;
  {
    ifstream srcFile(sourceFile);
    if (srcFile.fail()) {
      Logger::fmtLog(LogLevel::Error, "Failed to open source file: %s",
                     sourceFile.c_str());
      return 1;
    }
    src = string((istreambuf_iterator<char>(srcFile)),
                 istreambuf_iterator<char>());
  }

  // Lexical analysis
  Lexer asmLexer(src);
  vector<Token> tokens = asmLexer.tokenize();

  // AST Parser
  Parser asmParser(tokens);
  ast::Ptr<ASTProgram> program = move(asmParser.parseProgram());
#ifdef DEBUG
  if (program)
    program->Print();
  else {
    Logger::fmtLog(LogLevel::Error, "Program was null!");
    return 1;
  }
#else
  if (!program) {
    Logger::fmtLog(LogLevel::Error, "Program was null!");
    return 1;
  }
#endif // DEBUG
  // TODO: Further compilation steps would go here (machine code gen, symbol
  // resolution)

  return 0;
}
