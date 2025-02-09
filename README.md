# hexedit

**hexedit** is a command-line tool for modifying the content of a binary file by replacing a portion of the file starting at a given position with a specified hexadecimal string. The program preserves the original file's access and modification times.

## Usage

### Parameters:
- **-pos X**: Starting position in the file (in bytes) for the modification.
- **-w HEXDATA**: Hexadecimal string (must be an even number of characters, max length: 1000 characters).
- **-r SOURCE_FILE**: Path to the source file.
- **-o OUTPUT_FILE**: Path to the output file.

### Example:

'exedit -pos 150 -w "0020FF10aa" -r inputfile -o outputfile'

## Features
- Supports up to 1000-character hexadecimal data input.
- Preserves the original file’s access and modification timestamps.
- Secure handling: sensitive data is cleared from memory after processing.

## System Requirements
- MacOs
- Apple Silicon

## Installation and Compilation
Compile the program with the following command: 

'clang++ -std=c++17 -target arm64-apple-macos12 -O3 -flto -mcpu=apple-m1 -o hexedit hexedit.cpp'

## License
This project is licensed under the [MIT License](LICENSE).