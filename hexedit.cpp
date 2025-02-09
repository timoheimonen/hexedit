/*
 * Copyright (c) 2025 Timo Heimonen <timo.heimonen@gmail.com>
 *
 * This source code is licensed under the MIT License.
 * See the LICENSE file for full license details.
 * 
 * File: hexedit.cpp
 * Description:
 *   This program modifies a binary file by overwriting its content at a specified
 *   position with data provided as a hexadecimal string. The source file is read
 *   entirely into memory, modified, and then written to a new destination file.
 *   Additionally, the original file's access and modification timestamps are preserved.
 *
 * Usage:
 *   hexedit -pos X -w "HEXDATA" -r SOURCE_FILE -o OUTPUT_FILE
 *
 * Where:
 *   -pos    X         Starting byte position where the modifications begin.
 *   -w     HEXDATA    Hexadecimal string (must have an even number of characters and a maximum length of 1000 characters).
 *   -r     SOURCE_FILE Path to the source file.
 *   -o     OUTPUT_FILE Path for the output file.
 */

 #include <iostream>
 #include <fstream>
 #include <string>
 #include <vector>
 #include <cstdlib>    // std::strtol
 #include <sys/stat.h>
 #include <sys/time.h> // utimes
 #include <unistd.h>   // stat, utimes
 #include <stdexcept>
 
 // Securely zero out a memory area to prevent sensitive data from lingering.
 // Using a volatile pointer ensures that the compiler does not optimize away this loop.
 static void secureZero(void* ptr, std::size_t len) {
     volatile unsigned char* p = reinterpret_cast<volatile unsigned char*>(ptr);
     while (len--) {
         *p++ = 0;
     }
 }
 
 // Converts a hexadecimal string into a vector of binary bytes.
 // Example: "0102030A" -> {0x01, 0x02, 0x03, 0x0A}.
 // Throws std::runtime_error if the string length is odd or if an invalid character is found.
 std::vector<unsigned char> parseHexString(const std::string& hexStr)
 {
     if (hexStr.size() % 2 != 0) {
         throw std::runtime_error("Hex string length is odd.");
     }
 
     std::vector<unsigned char> result;
     result.reserve(hexStr.size() / 2);
 
     auto hexVal = [&](char c) -> int {
         if (c >= '0' && c <= '9') return c - '0';
         if (c >= 'A' && c <= 'F') return c - 'A' + 10;
         if (c >= 'a' && c <= 'f') return c - 'a' + 10;
         throw std::runtime_error("Invalid hex character: " + std::string(1, c));
     };
 
     for (std::size_t i = 0; i < hexStr.size(); i += 2) {
         int hi = hexVal(hexStr[i]);
         int lo = hexVal(hexStr[i+1]);
         unsigned char byteVal = static_cast<unsigned char>((hi << 4) | lo);
         result.push_back(byteVal);
     }
     return result;
 }
 
 int main(int argc, char* argv[])
 {
     // Expect 9 arguments: program name, -pos, position, -w, hex string, -r, source file, -o, destination file.
     if (argc != 9) {
         std::cerr << "Usage: " << argv[0]
                   << " -pos X -w \"HEXDATA\" -r SOURCE_FILE -o OUTPUT_FILE\n";
         return 1;
     }
 
     // Verify that flags are in the expected order.
     if (std::string(argv[1]) != "-pos" ||
         std::string(argv[3]) != "-w"   ||
         std::string(argv[5]) != "-r"   ||
         std::string(argv[7]) != "-o")
     {
         std::cerr << "Invalid parameter order.\n";
         return 1;
     }
 
     try {
         // Parse command-line parameters.
         long pos = std::strtol(argv[2], nullptr, 10);  // Starting position for modification.
         std::string hexStr = argv[4];                   // Hexadecimal string data.
         
         // Limit the HEX data to a maximum of 1000 characters.
         if (hexStr.size() > 1000) {
             throw std::runtime_error("HEX data length exceeds maximum allowed (1000 characters).");
         }
         
         std::string srcFile = argv[6];                  // Source file path.
         std::string dstFile = argv[8];                  // Destination file path.
 
         // Retrieve metadata (size, timestamps, etc.) of the source file.
         struct stat st {};
         if (stat(srcFile.c_str(), &st) != 0) {
             std::cerr << "Error: stat() failed for " << srcFile << "\n";
             return 1;
         }
         off_t fileSize = st.st_size;
 
         // Read the entire source file into a buffer.
         std::vector<char> buffer(fileSize);
         {
             std::ifstream fin(srcFile, std::ios::binary);
             if (!fin.is_open()) {
                 std::cerr << "Error: could not open file " << srcFile << "\n";
                 return 1;
             }
             fin.read(buffer.data(), fileSize);
             if (!fin) {
                 std::cerr << "Error: reading file " << srcFile << " failed\n";
                 return 1;
             }
         }
 
         // Convert the hex string to binary data.
         std::vector<unsigned char> binData = parseHexString(hexStr);
 
         // Securely clear the hex string from memory.
         secureZero(&hexStr[0], hexStr.size());
         hexStr.clear();
 
         // Overwrite the buffer with binary data starting at the specified position.
         if (pos >= 0 && static_cast<off_t>(pos) < fileSize) {
             for (std::size_t i = 0; i < binData.size(); ++i) {
                 off_t dstPos = pos + static_cast<off_t>(i);
                 if (dstPos >= fileSize) break;  // Prevent buffer overflow.
                 buffer[dstPos] = static_cast<char>(binData[i]);
             }
         }
 
         // Securely clear the binary data from memory.
         if (!binData.empty()) {
             secureZero(binData.data(), binData.size());
             binData.clear();
         }
 
         // Write the modified buffer to the destination file.
         {
             std::ofstream fout(dstFile, std::ios::binary | std::ios::trunc);
             if (!fout.is_open()) {
                 std::cerr << "Error: could not create file " << dstFile << "\n";
                 return 1;
             }
             fout.write(buffer.data(), fileSize);
             if (!fout) {
                 std::cerr << "Error: writing to file " << dstFile << " failed\n";
                 return 1;
             }
         }
 
         // Copy access and modification timestamps from the source file to the destination file.
         {
             struct timeval times[2];
             times[0].tv_sec  = st.st_atimespec.tv_sec;
             times[0].tv_usec = static_cast<suseconds_t>(st.st_atimespec.tv_nsec / 1000);
             times[1].tv_sec  = st.st_mtimespec.tv_sec;
             times[1].tv_usec = static_cast<suseconds_t>(st.st_mtimespec.tv_nsec / 1000);
 
             if (utimes(dstFile.c_str(), times) != 0) {
                 std::cerr << "Error: utimes failed for " << dstFile << "\n";
                 return 1;
             }
         }
 
         // Print a success message.
         std::cout << "File \"" << srcFile << "\" modified and saved as \"" << dstFile << "\"\n";
         return 0;
 
     } catch (const std::exception &ex) {
         std::cerr << "Error: " << ex.what() << "\n";
         return 1;
     }
 }