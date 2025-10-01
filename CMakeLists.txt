cmake_minimum_required(VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Compiler85)

# Include directories
include_directories("include")

# Sources
set(SRC_FILES
    src/Compiler85.cpp
    src/Logger.cpp
    src/asm_lexer.cpp
    src/asm_parser.cpp
    src/asm_generator.cpp
)

set(HEADER_FILES
    include/Compiler85.h
    include/Logger.h
    include/asm_lexer.h
    include/asm_parser.h
    include/ASTStructs.h
    include/asm_generator.h
    include/ProgramSerializer.h
)

# Create executable
add_executable(c85 ${SRC_FILES} ${HEADER_FILES})

# Set C++ standard
if(CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET c85 PROPERTY CXX_STANDARD 20)
endif()

# Add DEBUG macro for Debug builds
target_compile_definitions(c85 PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

# --- Python test integration ---
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Define test script and directories
set(TEST_SCRIPT ${CMAKE_SOURCE_DIR}/tests/run_tests.py)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)
set(APP_DIR $<TARGET_FILE_DIR:c85>)

# Run the Python script after build
add_custom_command(TARGET c85 POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${TEST_SCRIPT} ${TEST_DIR} ${APP_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Python acceptance tests..."
)
