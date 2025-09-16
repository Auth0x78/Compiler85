# 8085 Mini Compiler

A simple compiler for Intel 8085 opcodes.  
It takes 8085 assembly code as input and produces the corresponding machine code or raw binary as output.  
Designed as an educational project to explore compiler phases like lexing, parsing, code generation, and symbol resolution.  

## Build Instructions

```bash
git clone https://github.com/your-username/Compiler85.git
cd Compiler85
mkdir build && cd build
cmake ..
cmake --build .
````

## Usage

### Debug build

In **Debug mode**, the compiler is interactive — no command line arguments required.

```text
$>c85

Debug mode: No command line arguments required.
Enter the filepath of the source file:
Enter the filepath of the output file:
```

### Release build

In **Release mode**, the compiler expects arguments:

```bash
$>c85 <sourceFile> <outputFile> [-r]
```

* `<sourceFile>`: Path to input assembly file
* `<outputFile>`: Path where machine code will be written
* `-r` (optional): Output raw binary instead of default format

Example:

```bash
c85 examples/hello.asm build/hello.bin -r
```

## TODO Section

* [x] **Lexer** – tokenize assembly source
* [x] **Parser** – build AST from tokens
* [ ] **Code Generation** – lower AST into 8085 machine code
* [ ] **Symbol Resolution & Linking** – resolve labels, addresses, and forward references
* [ ] **Object File Generation** – outputs raw machine code or raw (hex-format) binary output to file

