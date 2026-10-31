#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

bool Platform_GetImageInfo(const void* data, size_t size, uint32_t* outW, uint32_t* outH, uint32_t* outFormat);
bool Platform_DecodeImage(const void* data, size_t size, uint32_t* outW, uint32_t* outH, std::vector<uint8_t>& outPixels);

// Writes a BGRA image as an uncompressed TGA (the format used by guild marks).
// Returns false when the file cannot be written.
bool Platform_SaveImageTGA(const char* path, int width, int height, const void* bgraPixels);
