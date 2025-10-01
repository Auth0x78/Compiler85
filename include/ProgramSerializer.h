#include <ASTStructs.h>
#include <asm_generator.h>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

class ProgramSerializer {
public:
  static void writeMemoryDumpView(const std::vector<BinaryBlock> &blocks,
                                  const std::string &filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
      throw std::runtime_error("Failed to open output file: " + filename);
    }

    for (const auto &block : blocks) {
      uint16_t addr = block.startAddr;
      for (uint8_t byte : block.code) {
        // Print address and byte in readable hex format
        out << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
            << addr << ": " << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte) << "\n";
        addr++;
      }
    }

    out.close();
  }

  static void writeRawBinary(const std::vector<BinaryBlock> &blocks,
                             const std::string &filename,
                             bool padGaps = false) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
      throw std::runtime_error("Failed to open file " + filename);
    }

    size_t currentAddr = 0;
    for (const auto &block : blocks) {
      if (padGaps && block.startAddr > currentAddr) {
        size_t gap = (size_t)block.startAddr - currentAddr;
        out.write(std::string(gap, '\0').c_str(), gap); // pad zeros
      }
      out.write(reinterpret_cast<const char *>(block.code.data()),
                block.code.size());
      currentAddr = block.startAddr + block.code.size();
    }

    out.close();
  }

  static void writeIntelHex(const std::vector<BinaryBlock> &blocks,
                            const std::string &filename) {
    std::ofstream out(filename);
    if (!out) {
      throw std::runtime_error("Failed to open file " + filename);
    }

    for (const auto &block : blocks) {
      uint16_t addr = block.startAddr;
      size_t remaining = block.code.size();
      size_t offset = 0;

      while (remaining > 0) {
        uint8_t recordLen = static_cast<uint8_t>(
            remaining > 16 ? 16 : remaining); // max 16 bytes per record
        std::stringstream ss;
        ss << ":" << std::uppercase << std::hex << std::setw(2)
           << std::setfill('0') << int(recordLen) << std::setw(4) << addr
           << "00"; // record type 00 = code

        uint8_t checksum = recordLen + (addr >> 8) + (addr & 0xFF) + 0x00;

        for (size_t i = 0; i < recordLen; i++) {
          uint8_t b = block.code[offset + i];
          ss << std::setw(2) << int(b);
          checksum += b;
        }

        checksum = (~checksum + 1) & 0xFF; // two's complement
        ss << std::setw(2) << int(checksum);

        out << ss.str() << "\n";

        remaining -= recordLen;
        offset += recordLen;
        addr += recordLen;
      }
    }

    // End-of-file record
    out << ":00000001FF\n";

    out.close();
  }
};
