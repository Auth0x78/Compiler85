// Compiler85.cpp : Defines the entry point for the application.
//
#include <Compiler85.h>
#include <filesystem>
using namespace std;

int main(int argc, char *argv[]) {
  string sourceFile;
  string CWD = std::filesystem::current_path().string() + "/";
  string outputFile;
  bool rawBinary = false;
  // new flag for MemoryDumpView
  bool memoryDumpView = false;

  // Help printer
  auto printHelp = []() {
    Logger::fmtLog(
        LogLevel::Info,
        "\nUsage: c85 <sourceFile> <outputFile> [options]\n"
        "Options:\n"
        "  -r        Outputs raw binary (else, default is Intel HEX format)\n"
        "  -d        Outputs human-readable memory dump view (address: byte "
        "per line)\n"
        "  -h, --help Show this help message\n"
        "\nExamples:\n"
        "  c85 program.asm program.hex\n"
        "  c85 program.asm program.bin -r\n"
        "  c85 program.asm program.dump -d\n");
  };

  // Handle no arguments or help flag
  if (argc < 3) {
#ifdef DEBUG
    Logger::fmtLog(LogLevel::Info,
                   "Debug mode: no command line arguments required.");
    Logger::fmtLog(LogLevel::Info, "Enter the filepath of the source file: ");
    cin >> sourceFile;
    Logger::fmtLog(LogLevel::Info, "Enter the filepath of the output file: ");
    cin >> outputFile;
    rawBinary = false;
    memoryDumpView = true;
#else
    printHelp();
    return 1;
#endif
  }

  // Handle explicit help request
  if (string(argv[1]) == "-h" || string(argv[1]) == "--help") {
    printHelp();
    return 0;
  }

  sourceFile = argv[1];
  outputFile = argv[2];

  // Parse optional flags
  for (int i = 3; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-r") {
      rawBinary = true;
    } else if (arg == "-d") {
      memoryDumpView = true;
    } else if (arg == "-h" || arg == "--help") {
      printHelp();
      return 0;
    } else {
      Logger::fmtLog(LogLevel::Warning, "Unknown option: %s", arg.c_str());
    }
  }

  // === Read source file ===
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

  // === Lexical analysis ===
  Lexer asmLexer(src);
  vector<Token> tokens = asmLexer.tokenize();

  // === AST Parser ===
  Parser asmParser(tokens);
  ast::Ptr<ASTProgram> program = move(asmParser.parseProgram());
  unordered_map symbolTable = move(asmParser.getSymbolTable());

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
#endif

  // === Code generation ===
  // Also includes symbol resolution too
  AsmGenerator generator(program, symbolTable);
  generator.GenerateBinary();

#ifdef DEBUG
  // write out generated machine code / HEX
  generator.Print(stdout, sourceFile.c_str());
  fflush(stdout);
#endif // DEBUG
  vector<BinaryBlock> blocks(move(generator.getCodeBlock()));

  if (rawBinary)
    ProgramSerializer::writeRawBinary(blocks, outputFile, true);
  else if (memoryDumpView)
    ProgramSerializer::writeMemoryDumpView(blocks, outputFile);
  else
    ProgramSerializer::writeIntelHex(blocks, outputFile);

  return 0;
}
