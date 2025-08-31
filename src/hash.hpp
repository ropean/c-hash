// hash.hpp - shared hashing utilities
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace hashcore {

namespace fs = std::filesystem;

struct Sha256Digest {
	std::array<unsigned char, 32> bytes{};
};

// Stream a file from disk and compute SHA-256 using Windows CNG (bcrypt).
// Returns true on success; on failure, out_error contains a short description.
bool compute_sha256_streamed(const fs::path &file_path,
	Sha256Digest &out_digest,
	uint64_t &out_size_bytes,
	double &out_elapsed_seconds,
	std::string &out_error);

// Convert digest to hex string (upper/lower per flag).
std::string to_hex(const Sha256Digest &digest, bool uppercase);

// Convert digest to Base64 string.
std::string to_base64(const Sha256Digest &digest);

}


