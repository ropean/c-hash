#include "hash.hpp"

#include <windows.h>
#include <bcrypt.h>
#include <vector>

#if defined(_MSC_VER)
#pragma comment(lib, "Bcrypt.lib")
#endif

namespace hashcore {

bool compute_sha256_streamed(const fs::path &file_path, Sha256Digest &out_digest, uint64_t &out_size_bytes, double &out_elapsed_seconds, std::string &out_error) {
	HANDLE file = CreateFileW(file_path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		out_error = "Failed to open file";
		return false;
	}

	LARGE_INTEGER size{};
	if (!GetFileSizeEx(file, &size)) {
		CloseHandle(file);
		out_error = "Failed to get file size";
		return false;
	}
	out_size_bytes = static_cast<uint64_t>(size.QuadPart);

	BCRYPT_ALG_HANDLE alg_handle = nullptr;
	BCRYPT_HASH_HANDLE hash_handle = nullptr;
	DWORD hash_object_len = 0, data_len = 0;

	if (BCryptOpenAlgorithmProvider(&alg_handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
		CloseHandle(file);
		out_error = "BCryptOpenAlgorithmProvider failed";
		return false;
	}
	if (BCryptGetProperty(alg_handle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hash_object_len, sizeof(hash_object_len), &data_len, 0) != 0) {
		BCryptCloseAlgorithmProvider(alg_handle, 0);
		CloseHandle(file);
		out_error = "BCryptGetProperty(ObjectLength) failed";
		return false;
	}

	std::vector<UCHAR> hash_object(hash_object_len);
	if (BCryptCreateHash(alg_handle, &hash_handle, hash_object.data(), static_cast<ULONG>(hash_object.size()), nullptr, 0, 0) != 0) {
		BCryptCloseAlgorithmProvider(alg_handle, 0);
		CloseHandle(file);
		out_error = "BCryptCreateHash failed";
		return false;
	}

	const size_t buffer_size = 1 * 1024 * 1024;
	std::vector<unsigned char> buffer(buffer_size);

	LARGE_INTEGER start{}, end{};
	QueryPerformanceCounter(&start);
	LARGE_INTEGER freq{};
	QueryPerformanceFrequency(&freq);

	DWORD bytes_read = 0;
	BOOL read_ok = FALSE;
	do {
		read_ok = ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr);
		if (!read_ok) {
			BCryptDestroyHash(hash_handle);
			BCryptCloseAlgorithmProvider(alg_handle, 0);
			CloseHandle(file);
			out_error = "ReadFile failed";
			return false;
		}
		if (bytes_read > 0) {
			if (BCryptHashData(hash_handle, buffer.data(), bytes_read, 0) != 0) {
				BCryptDestroyHash(hash_handle);
				BCryptCloseAlgorithmProvider(alg_handle, 0);
				CloseHandle(file);
				out_error = "BCryptHashData failed";
				return false;
			}
		}
	} while (bytes_read > 0);

	DWORD hash_len = 0;
	data_len = 0;
	if (BCryptGetProperty(alg_handle, BCRYPT_HASH_LENGTH, (PUCHAR)&hash_len, sizeof(hash_len), &data_len, 0) != 0 || hash_len != out_digest.bytes.size()) {
		BCryptDestroyHash(hash_handle);
		BCryptCloseAlgorithmProvider(alg_handle, 0);
		CloseHandle(file);
		out_error = "BCryptGetProperty(HashLength) failed";
		return false;
	}

	if (BCryptFinishHash(hash_handle, out_digest.bytes.data(), static_cast<ULONG>(out_digest.bytes.size()), 0) != 0) {
		BCryptDestroyHash(hash_handle);
		BCryptCloseAlgorithmProvider(alg_handle, 0);
		CloseHandle(file);
		out_error = "BCryptFinishHash failed";
		return false;
	}

	QueryPerformanceCounter(&end);
	out_elapsed_seconds = static_cast<double>(end.QuadPart - start.QuadPart) / static_cast<double>(freq.QuadPart);

	BCryptDestroyHash(hash_handle);
	BCryptCloseAlgorithmProvider(alg_handle, 0);
	CloseHandle(file);
	return true;
}

std::string to_hex(const Sha256Digest &digest, bool uppercase) {
	static const char *lower = "0123456789abcdef";
	static const char *upper = "0123456789ABCDEF";
	const char *digits = uppercase ? upper : lower;

	std::string out;
	out.resize(digest.bytes.size() * 2);
	for (size_t i = 0; i < digest.bytes.size(); ++i) {
		unsigned char b = digest.bytes[i];
		out[i * 2] = digits[(b >> 4) & 0x0F];
		out[i * 2 + 1] = digits[b & 0x0F];
	}
	return out;
}

std::string to_base64(const Sha256Digest &digest) {
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	const unsigned char *data = digest.bytes.data();
	size_t len = digest.bytes.size();
	out.reserve(((len + 2) / 3) * 4);
	for (size_t i = 0; i < len; i += 3) {
		unsigned int octet_a = i < len ? data[i] : 0;
		unsigned int octet_b = (i + 1) < len ? data[i + 1] : 0;
		unsigned int octet_c = (i + 2) < len ? data[i + 2] : 0;
		unsigned int triple = (octet_a << 16) | (octet_b << 8) | (octet_c);
		out.push_back(table[(triple >> 18) & 0x3F]);
		out.push_back(table[(triple >> 12) & 0x3F]);
		out.push_back((i + 1) < len ? table[(triple >> 6) & 0x3F] : '=');
		out.push_back((i + 2) < len ? table[(triple) & 0x3F] : '=');
	}
	return out;
}

}


