import os
import sys

def get_multiline_input(prompt: str) -> str:
    print(f"Enter {prompt} (finish with Ctrl+D on Linux/Mac or Ctrl+Z on Windows then Enter):")
    try:
        return sys.stdin.read().strip()
    except KeyboardInterrupt:
        print("\nInput cancelled.")
        sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_tests.py <test_case_name>")
        sys.exit(1)

    test_name = sys.argv[1]

    # Directories
    input_dir = "./test_input"
    expect_dir = "./test_expect"
    os.makedirs(input_dir, exist_ok=True)
    os.makedirs(expect_dir, exist_ok=True)

    # File paths
    asm_file = os.path.join(input_dir, f"test_{test_name}.asm")
    dump_file = os.path.join(expect_dir, f"expect_{test_name}.dump")

    print(f"Generating test case: {test_name}\n")

    print("=== Assembly (.asm) file ===")
    asm_content = get_multiline_input("assembly code")
    print("\n=== Golden dump (.dump) file ===")
    dump_content = get_multiline_input("golden dump")

    # Write files
    with open(asm_file, "w") as f:
        f.write(asm_content + "\n")
    with open(dump_file, "w") as f:
        f.write(dump_content + "\n")

    print(f"\n✅ Test case '{test_name}' generated successfully!")
    print(f"  - {asm_file}")
    print(f"  - {dump_file}")

if __name__ == "__main__":
    main()
