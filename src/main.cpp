#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cwchar>
#include "hash.hpp"

namespace fs = std::filesystem;

static void print_usage() {
	std::cout << "c-hash v0.1.0\n";
	std::cout << "Usage: c-hash [-u] <file_path>\n";
	std::cout << "  -u    Uppercase HEX output\n";
	std::cout << "Outputs: HEX, Base64, size, elapsed, throughput\n";
}

int wmain(int argc, wchar_t **argv) {
	if (argc < 2) {
		print_usage();
		return 1;
	}

	bool uppercase_hex = false;
	int argi = 1;
	if (std::wcscmp(argv[argi], L"-u") == 0 || std::wcscmp(argv[argi], L"--uppercase") == 0) {
		uppercase_hex = true;
		++argi;
	}
	if (argi >= argc) {
		print_usage();
		return 1;
	}

	fs::path path = argv[argi];
	if (!fs::exists(path) || !fs::is_regular_file(path)) {
		std::wcerr << L"File not found: " << path.wstring() << L"\n";
		return 2;
	}

	hashcore::Sha256Digest digest{};
	uint64_t size_bytes = 0;
	double elapsed_s = 0.0;
	std::string error;
	if (!hashcore::compute_sha256_streamed(path, digest, size_bytes, elapsed_s, error)) {
		std::cerr << "Error: " << error << "\n";
		return 3;
	}

	std::string hex = hashcore::to_hex(digest, uppercase_hex);
	std::string b64 = hashcore::to_base64(digest);
	double mb = static_cast<double>(size_bytes) / (1024.0 * 1024.0);
	double throughput = elapsed_s > 0.0 ? (mb / elapsed_s) : 0.0;

	std::wcout << L"Path: " << path.wstring() << L"\n";
	std::cout << "Size: " << size_bytes << " bytes\n";
	std::cout << "Elapsed: " << elapsed_s << " s\n";
	std::cout << "Throughput: " << throughput << " MiB/s\n";
	std::cout << "HEX: " << hex << "\n";
	std::cout << "Base64: " << b64 << "\n";
	return 0;
}


