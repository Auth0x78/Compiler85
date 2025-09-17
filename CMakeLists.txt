# CMakeList.txt : CMake project for Compiler85, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Compiler85")

# Include directories
include_directories("include")

# Add source to this project's executable.
add_executable (Compiler85 "src/Compiler85.cpp" "include/Compiler85.h" "include/Logger.h" "src/Logger.cpp" "include/asm_lexer.h" "src/asm_lexer.cpp" "include/asm_parser.h" "src/asm_parser.cpp" "include/ASTStructs.h" "include/asm_generator.h" "src/asm_generator.cpp")

# Keep project name "Compiler85" but rename binary to "c85"
set_target_properties(Compiler85 PROPERTIES OUTPUT_NAME "c85")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Compiler85 PROPERTY CXX_STANDARD 20)
endif()

# Add DEBUG macro depending on build configuration
target_compile_definitions(Compiler85 PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)
