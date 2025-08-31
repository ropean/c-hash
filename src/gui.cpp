#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <string_view>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include "hash.hpp"

namespace fs = std::filesystem;

static const wchar_t *kWndClass = L"CHashGuiWindow";

struct UiState {
	std::wstring selectedPath;
	bool uppercaseHex = false;
	std::wstring hexOut;
	std::wstring b64Out;
	std::wstring sizeOut;
	std::wstring elapsedOut;
	std::wstring throughputOut;
	bool isHashing = false;
	bool hasDigest = false;
	hashcore::Sha256Digest lastDigest{};
	uint64_t lastSizeBytes = 0;
	double lastElapsedSeconds = 0.0;
};

static UiState g_state{};
static const UINT WM_HASH_DONE = WM_APP + 1;

struct HashResult {
	bool success = false;
	std::wstring path;
	hashcore::Sha256Digest digest{};
	uint64_t sizeBytes = 0;
	double elapsedSeconds = 0.0;
	std::string error;
};

static void SetControlText(HWND hwnd, int ctrlId, const std::wstring &text) {
	SetWindowTextW(GetDlgItem(hwnd, ctrlId), text.c_str());
}

static void SetUiBusy(HWND hwnd, bool busy) {
	int interactiveIds[] = {100, 200, 202, 203, 101, 102, 204, 205};
	for (int id : interactiveIds) {
		HWND h = GetDlgItem(hwnd, id);
		if (h) EnableWindow(h, busy ? FALSE : TRUE);
	}
	if (busy) {
		SetWindowTextW(hwnd, L"c-hash — Hashing...");
	} else {
		SetWindowTextW(hwnd, L"c-hash");
	}
}

static std::wstring Utf8ToWide(const std::string &s) {
	if (s.empty()) return L"";
	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	std::wstring w(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
	return w;
}

static std::wstring InsertThousandsSeparators(const std::wstring &digits) {
	if (digits.empty()) return digits;
	std::wstring intpart = digits;
	bool neg = false;
	if (intpart.size() > 0 && intpart[0] == L'-') {
		neg = true;
		intpart.erase(intpart.begin());
	}
	std::wstring out;
	int count = 0;
	for (int i = (int)intpart.size() - 1; i >= 0; --i) {
		out.push_back(intpart[(size_t)i]);
		++count;
		if (count == 3 && i > 0) {
			out.push_back(L',');
			count = 0;
		}
	}
	std::reverse(out.begin(), out.end());
	if (neg) out.insert(out.begin(), L'-');
	return out;
}

static std::wstring FormatNumberWithCommas(double value, int decimals) {
	std::wstringstream ss;
	ss.setf(std::ios::fixed); ss << std::setprecision(decimals) << value;
	std::wstring s = ss.str();
	size_t dot = s.find(L'.');
	std::wstring intpart = dot == std::wstring::npos ? s : s.substr(0, dot);
	std::wstring fracpart = dot == std::wstring::npos ? L"" : s.substr(dot + 1);
	intpart = InsertThousandsSeparators(intpart);
	if (!fracpart.empty()) {
		// trim trailing zeros
		while (!fracpart.empty() && fracpart.back() == L'0') fracpart.pop_back();
	}
	if (fracpart.empty()) return intpart;
	return intpart + L"." + fracpart;
}

static std::wstring FormatSize(uint64_t bytes) {
	const double b = (double)bytes;
	if (b >= 1e9) {
		double v = b / 1e9;
		return FormatNumberWithCommas(v, 2) + L" GB";
	} else if (b >= 1e6) {
		double v = b / 1e6;
		return FormatNumberWithCommas(v, 2) + L" MB";
	} else {
		return InsertThousandsSeparators(std::to_wstring(bytes)) + L" bytes";
	}
}

static std::wstring FormatElapsed(double seconds) {
	if (seconds < 1.0) {
		double ms = seconds * 1000.0;
		return FormatNumberWithCommas(ms, 2) + L" ms";
	} else if (seconds < 60.0) {
		return FormatNumberWithCommas(seconds, 2) + L" s";
	} else if (seconds < 3600.0) {
		double m = seconds / 60.0;
		return FormatNumberWithCommas(m, 2) + L" m";
	} else {
		double h = seconds / 3600.0;
		return FormatNumberWithCommas(h, 2) + L" h";
	}
}

static std::wstring FormatThroughput(uint64_t bytes, double seconds) {
	if (seconds <= 0.0) return L"0 KB/s";
	double bps = (double)bytes / seconds; // bytes per second
	if (bps >= 1e9) {
		double v = bps / 1e9;
		return FormatNumberWithCommas(v, 2) + L" GB/s";
	} else if (bps >= 1e6) {
		double v = bps / 1e6;
		return FormatNumberWithCommas(v, 2) + L" MB/s";
	} else {
		double v = bps / 1e3;
		return FormatNumberWithCommas(v, 2) + L" KB/s";
	}
}

static void ApplyOutputs(HWND hwnd) {
	std::string hex = hashcore::to_hex(g_state.lastDigest, g_state.uppercaseHex);
	std::string b64 = hashcore::to_base64(g_state.lastDigest);
	g_state.hexOut = Utf8ToWide(hex);
	g_state.b64Out = Utf8ToWide(b64);
	g_state.sizeOut = FormatSize(g_state.lastSizeBytes);
	g_state.elapsedOut = FormatElapsed(g_state.lastElapsedSeconds);
	g_state.throughputOut = FormatThroughput(g_state.lastSizeBytes, g_state.lastElapsedSeconds);
	SetControlText(hwnd, 101, g_state.hexOut);
	SetControlText(hwnd, 102, g_state.b64Out);
	SetControlText(hwnd, 103, g_state.sizeOut);
	SetControlText(hwnd, 104, g_state.elapsedOut);
	SetControlText(hwnd, 105, g_state.throughputOut);
}

static void StartHash(HWND hwnd) {
	if (g_state.selectedPath.empty() || g_state.isHashing) return;
	g_state.isHashing = true;
	g_state.hasDigest = false;
	std::wstring path = g_state.selectedPath;
	SetUiBusy(hwnd, true);
	std::thread([hwnd, path]() {
		HashResult *res = new HashResult();
		res->path = path;
		std::string err;
		if (!hashcore::compute_sha256_streamed(path, res->digest, res->sizeBytes, res->elapsedSeconds, err)) {
			res->success = false;
			res->error = err;
		} else {
			res->success = true;
		}
		PostMessageW(hwnd, WM_HASH_DONE, 0, (LPARAM)res);
	}).detach();
}

static void BrowseFile(HWND hwnd) {
	OPENFILENAMEW ofn{};
	wchar_t path[MAX_PATH] = L"";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"All Files\0*.*\0";
	ofn.lpstrFile = path;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileNameW(&ofn)) {
		g_state.selectedPath = path;
		SetControlText(hwnd, 100, g_state.selectedPath);
		StartHash(hwnd);
	}
}

static void CopyToClipboard(HWND hwnd, const std::wstring &text) {
	if (!OpenClipboard(hwnd)) return;
	EmptyClipboard();
	size_t bytes = (text.size() + 1) * sizeof(wchar_t);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (hMem) {
		void *dst = GlobalLock(hMem);
		memcpy(dst, text.c_str(), bytes);
		GlobalUnlock(hMem);
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		CreateWindowW(L"STATIC", L"Path:", WS_CHILD | WS_VISIBLE, 10, 10, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 100, 10, 470, 24, hwnd, (HMENU)100, nullptr, nullptr);
		CreateWindowW(L"BUTTON", L"Browse", WS_CHILD | WS_VISIBLE, 580, 10, 60, 24, hwnd, (HMENU)200, nullptr, nullptr);
		CreateWindowW(L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE, 650, 10, 60, 24, hwnd, (HMENU)202, nullptr, nullptr);
		CreateWindowW(L"BUTTON", L"Uppercase HEX", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 100, 44, 130, 20, hwnd, (HMENU)203, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"HEX:", WS_CHILD | WS_VISIBLE, 10, 74, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, 100, 74, 550, 24, hwnd, (HMENU)101, nullptr, nullptr);
		CreateWindowW(L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE, 660, 74, 60, 24, hwnd, (HMENU)204, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"Base64:", WS_CHILD | WS_VISIBLE, 10, 104, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, 100, 104, 550, 24, hwnd, (HMENU)102, nullptr, nullptr);
		CreateWindowW(L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE, 660, 104, 60, 24, hwnd, (HMENU)205, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"Size:", WS_CHILD | WS_VISIBLE, 10, 134, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 100, 134, 300, 20, hwnd, (HMENU)103, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"Elapsed:", WS_CHILD | WS_VISIBLE, 10, 154, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 100, 154, 300, 20, hwnd, (HMENU)104, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"Throughput:", WS_CHILD | WS_VISIBLE, 10, 174, 80, 20, hwnd, nullptr, nullptr, nullptr);
		CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 100, 174, 300, 20, hwnd, (HMENU)105, nullptr, nullptr);

		DragAcceptFiles(hwnd, TRUE);
		return 0;
	}
	case WM_COMMAND: {
		int id = LOWORD(wParam);
		if (id == 200) { // Browse
			BrowseFile(hwnd);
		} else if (id == 202) { // Clear
			g_state = UiState{};
			SetControlText(hwnd, 100, L"");
			SetControlText(hwnd, 101, L"");
			SetControlText(hwnd, 102, L"");
			SetControlText(hwnd, 103, L"");
			SetControlText(hwnd, 104, L"");
			SetControlText(hwnd, 105, L"");
		} else if (id == 203) { // Uppercase toggle
			g_state.uppercaseHex = (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if (g_state.hasDigest) {
				std::string hex = hashcore::to_hex(g_state.lastDigest, g_state.uppercaseHex);
				g_state.hexOut = Utf8ToWide(hex);
				SetControlText(hwnd, 101, g_state.hexOut);
			}
		} else if (id == 204) { // Copy HEX
			CopyToClipboard(hwnd, g_state.hexOut);
		} else if (id == 205) { // Copy Base64
			CopyToClipboard(hwnd, g_state.b64Out);
		}
		return 0;
	}
	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		wchar_t path[MAX_PATH];
		if (g_state.isHashing) {
			DragFinish(hDrop);
			return 0;
		}
		if (DragQueryFileW(hDrop, 0, path, MAX_PATH)) {
			g_state.selectedPath = path;
			SetControlText(hwnd, 100, g_state.selectedPath);
			StartHash(hwnd);
		}
		DragFinish(hDrop);
		return 0;
	}
	case WM_HASH_DONE: {
		HashResult *res = (HashResult*)lParam;
		g_state.isHashing = false;
		if (res) {
			if (res->success) {
				g_state.lastDigest = res->digest;
				g_state.lastSizeBytes = res->sizeBytes;
				g_state.lastElapsedSeconds = res->elapsedSeconds;
				g_state.hasDigest = true;
				ApplyOutputs(hwnd);
			} else {
				MessageBoxW(hwnd, Utf8ToWide(res->error).c_str(), L"Error", MB_ICONERROR);
			}
			delete res;
		}
		SetUiBusy(hwnd, false);
		return 0;
	}
	case WM_CTLCOLORSTATIC: {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)(HBRUSH)(COLOR_WINDOW+1);
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
	WNDCLASSW wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = kWndClass;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClassW(&wc);

	HWND hwnd = CreateWindowExW(0, kWndClass, L"c-hash", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 740, 240, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd) return 1;
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}


