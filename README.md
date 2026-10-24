# EterBase (`pkg_eterbase`)

Core foundation and systems library for the Metin2 Linux port. Provides low-level operating system abstraction, memory management, compression, encryption, and utility algorithms.

[![EterBase CI](https://github.com/open-mt2/pkg_eterbase/actions/workflows/ci.yml/badge.svg)](https://github.com/open-mt2/pkg_eterbase/actions)

---

## Features

- **POSIX Platform Abstraction**: Win32-compatible types (`DWORD`, `BYTE`, `BOOL`, `HANDLE`, etc.) mapped to standard POSIX types on 64-bit Linux (`x86_64`).
- **Compression**: `CLZO` / `CLZObject` wrapping LZO 2.x for high-throughput block compression and decompression.
- **Encryption**: 128-bit block TEA (Tiny Encryption Algorithm) with roundtrip verification and buffer padding.
- **Hashing**: Fast 32-bit CRC calculation (`GetCRC32`, `GetCaseCRC32`).
- **File I/O**: `CFileBase` POSIX file descriptor wrapper and `CMappedFile` high-performance `mmap()`/`munmap()` memory-mapped file reader.
- **Timer Subsystem**: `CTimer` and `ELTimer` backed by `std::chrono::steady_clock` monotonic timestamps.
- **Expression Evaluator**: `CPoly` mathematical expression parsing, tokenization, and dynamic variable evaluation.
- **String & Container Utilities**: `split_string`, `stl_lowers`, `korean_tolower`, and custom STL algorithms.

---

## Building

### Requirements
- CMake 3.20+
- GCC 11+ or Clang 14+ (C++17 support)
- Ninja or Make
- `liblzo2` development headers (`liblzo2-dev` on Debian/Ubuntu, `lzo` on Arch Linux)

### Commands
```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# Compile
cmake --build build

# Run unit tests
ctest --test-dir build --output-on-failure
```

---

## Test Suite

The library includes 7 automated unit tests covering all core modules:

| Test Target | Component | Coverage |
| :--- | :--- | :--- |
| `test_tea` | `tea.h` / `tea.cpp` | 128-bit key block encryption, decryption, and buffer padding |
| `test_crc32` | `CRC32.h` / `CRC32.cpp` | Consistency, string hashes, case distinction |
| `test_lzo` | `lzo.h` / `lzo.cpp` | Plaintext and encrypted compression/decompression |
| `test_file` | `FileBase.h`, `MappedFile.h` | Temporary file write, read back, and POSIX `mmap()` mapping |
| `test_stl` | `Stl.h` / `Stl.cpp` | String splitting, lowering, and character conversion |
| `test_timer` | `Timer.h` / `Timer.cpp` | Monotonic clock advance, millisecond resolution, and delta calculation |
| `test_poly` | `Poly.h` / `Poly.cpp` | Arithmetic order of operations, parenthesis grouping, and variables |

---

## Integration

### As a CMake Subdirectory
```cmake
add_subdirectory(extern/pkg_eterbase)
target_link_libraries(YourTarget PRIVATE EterBase)
```
Include paths (`EterBase`, `EterBase/Poly`, and `Platform`) are automatically exported via `EterBase`'s public interface.
