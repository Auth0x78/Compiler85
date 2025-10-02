import sys
import os
import subprocess
import time
import re

# ANSI color codes
class Colors:
    HEADER = '\033[95m'
    TITLE = '\033[96m'
    TESTNAME = '\033[93m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_progress(current, total, bar_length=40):
    fraction = current / total
    filled_length = int(bar_length * fraction)
    bar = '#' * filled_length + '-' * (bar_length - filled_length)
    print(f'\rProgress: |{bar}| {current}/{total} tests', end='', flush=True)

def main():
    if len(sys.argv) < 3:
        print("Usage: run_tests.py <test_dir> <app_dir>")
        return 1

    test_dir = sys.argv[1]
    app_dir = sys.argv[2]

    test_src_dir = os.path.join(test_dir, "test_input")
    test_expect_dir = os.path.join(test_dir, "test_expect")
    test_output_dir = os.path.join(test_dir, "test_out")

    # Path to the c85 executable
    c85_exe = os.path.join(app_dir, "c85")
    if os.name == 'nt':
        c85_exe += ".exe"

    if not os.path.isfile(c85_exe):
        print(f"{Colors.FAIL}Error: Compiler executable not found at {c85_exe}{Colors.RESET}")
        return 1

    # Gather test files
    test_files = [f for f in os.listdir(test_src_dir) if f.startswith("test_") and f.endswith(".asm")]
    total_tests = len(test_files)
    passed_tests = 0
    failed_tests = 0

    print(f"{Colors.TITLE}{Colors.BOLD}{'='*10} Running Compiler85 System Tests {'='*10}{Colors.RESET}")
    print(f"{Colors.CYAN}Test directory: {test_dir}")
    print(f"Compiler executable: {c85_exe}{Colors.RESET}\n")

    start_time = time.time()

    for idx, fname in enumerate(test_files, 1):
        test_name = fname[5:-4]  # strip "test_" prefix and ".asm" suffix
        print(f"\n\n{Colors.TESTNAME}Running Test: {test_name}{Colors.RESET}")

        src_file = os.path.join(test_src_dir, fname)
        output_file = os.path.join(test_output_dir, f"{test_name}.to")
        golden_file = os.path.join(test_expect_dir, f"expect_{test_name}.dump")

        if not os.path.isfile(golden_file):
            print(f"{Colors.WARNING}No golden file for {test_name}, skipping.{Colors.RESET}\n")
            continue

        # Read golden file
        with open(golden_file, "r") as gf:
            lines = gf.readlines()
            expected_return = int(lines[0].strip())
            expected_output = "".join(lines[1:]).strip()

        passed = True
        test_start = time.time()
        try:
            # Run compiler
            result = subprocess.run([c85_exe, src_file, output_file, "-d"],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True)
            actual_return = result.returncode
            actual_stdout = result.stdout.strip()
            actual_stderr = result.stderr.strip()
            actual_combined = re.sub(r'\x1B\[[0-?]*[ -/]*[@-~]', '', (actual_stdout + "\n" + actual_stderr)).strip()

            # Show stdout and stderr
            if actual_stdout:
                print(f"{Colors.CYAN}Compiler stdout:\n{actual_stdout}{Colors.RESET}")
            if actual_stderr:
                print(f"{Colors.WARNING}Compiler stderr:\n{actual_stderr}{Colors.RESET}")

            # Check return code
            if actual_return != expected_return:
                print(f"{Colors.FAIL}FAIL: Return code mismatch{Colors.RESET}")
                print(f"Expected: {expected_return}, Got: {actual_return}")
                passed = False
            else:
                # Check output for success cases
                if expected_return == 0:
                    if os.path.isfile(output_file):
                        with open(output_file, "r") as out_f:
                            out_data = out_f.read().strip()
                        if out_data != expected_output:
                            print(f"{Colors.FAIL}FAIL: Output mismatch{Colors.RESET}")
                            print(f"Expected:\n{expected_output}\nGot:\n{out_data}")
                            passed = False
                    else:
                        print(f"{Colors.FAIL}FAIL: Output file missing{Colors.RESET}")
                        passed = False
                else:
                    # Error cases
                    if actual_combined.strip().replace('\r\n', '\n') != expected_output.strip().replace('\r\n', '\n'):
                        print(f"{Colors.FAIL}FAIL: Error output mismatch{Colors.RESET}")
                        print(f"Expected:\n{expected_output}\nGot:\n{actual_combined}")
                        passed = False

        except Exception as e:
            print(f"{Colors.FAIL}Exception while running compiler: {e}{Colors.RESET}")
            passed = False
        finally:
            if passed:
                if os.path.isfile(output_file):
                    os.remove(output_file)

        test_end = time.time()
        duration = test_end - test_start

        # Update stats
        if passed:
            passed_tests += 1
            status = f"{Colors.OKGREEN}PASS{Colors.RESET}"
        else:
            failed_tests += 1
            status = f"{Colors.FAIL}FAIL{Colors.RESET}"

        print(f"Test {test_name}: {status} (Time: {duration:.3f}s)\n")
        print_progress(idx, total_tests)

    end_time = time.time()
    total_duration = end_time - start_time
    print("\n")
    print(f"{Colors.BOLD}{'='*10} Test Summary {'='*10}{Colors.RESET}")
    print(f"{Colors.OKGREEN}Passed: {passed_tests}{Colors.RESET}")
    print(f"{Colors.FAIL}Failed: {failed_tests}{Colors.RESET}")
    print(f"Total: {total_tests}")
    print(f"Total time taken: {total_duration:.2f} seconds")

    return 0

if __name__ == "__main__":
    sys.exit(main())
