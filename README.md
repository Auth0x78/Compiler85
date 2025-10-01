# Compiler 8085

A simple compiler for Intel 8085 opcodes.  
It takes 8085 assembly code as input and produces the corresponding machine code or raw binary as output.  
Designed as an educational project to explore compiler phases like lexing, parsing, code generation, and symbol resolution.  

## Build Instructions

```bash
git clone https://github.com/Auth0x78/Compiler85.git
cd Compiler85
mkdir build && cd build
cmake ..
cmake --build .
````

## Usage

### Debug build

In **Debug mode**, the compiler is interactive — no command line arguments required.

```text
$> c85

Debug mode: No command line arguments required.
Enter the filepath of the source file:
Enter the filepath of the output file:

```

Here’s an updated and polished version of your README section:

---

### Release Build

In **Release mode**, the compiler is run from the command line with the following syntax:

```bash
$> c85 <sourceFile> <outputFile> [options]
```

#### Arguments

* `<sourceFile>`: Path to the input 8085 assembly (`.asm`) file. Can be relative or absolute.
* `<outputFile>`: Path where the compiled machine code will be written. Can be relative or absolute.
* `[options]` (optional flags):

  | Flag           | Description                                                        |
  | -------------- | ------------------------------------------------------------------ |
  | `-r`           | Output raw binary (`.bin`) instead of the default Intel HEX format |
  | `-d`           | Generate a human-readable memory dump of the compiled program      |
  | `-h`, `--help` | Show this help message                                             |

#### Examples

Generate Intel HEX (default):

```bash
$> c85 program.asm program.hex
```

Generate raw binary:

```bash
$> c85 program.asm program.bin -r
```

Generate memory dump for inspection:

```bash
$> c85 program.asm program.dump -d
```

**Notes:**
* If an output file already exists, it will be **overwritten**.
---

Example:

```bash
c85 examples/hello.asm build/hello.bin -r
```

## TODO Section

* [x] **Lexer** – tokenize assembly source
* [x] **Parser** – build AST from tokens
* [x] **Code Generation** – lower AST into 8085 machine code
* [x] **Symbol Resolution & Linking** – resolve labels, addresses, and forward references
* [x] **Object File Generation** – outputs raw machine code or raw (hex-format) binary output to file
* [ ] **Test Case & Output Verification** - verify output against known test cases

