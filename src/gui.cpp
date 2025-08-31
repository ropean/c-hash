#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <string_view>
#include <filesystem>
#include <sstream>
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
};

static UiState g_state{};

static void SetControlText(HWND hwnd, int ctrlId, const std::wstring &text) {
	SetWindowTextW(GetDlgItem(hwnd, ctrlId), text.c_str());
}

static std::wstring Utf8ToWide(const std::string &s) {
	if (s.empty()) return L"";
	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	std::wstring w(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
	return w;
}

static void DoHash(HWND hwnd) {
	if (g_state.selectedPath.empty()) return;
	hashcore::Sha256Digest digest{};
	uint64_t sizeBytes = 0;
	double elapsed = 0.0;
	std::string err;
	if (!hashcore::compute_sha256_streamed(g_state.selectedPath, digest, sizeBytes, elapsed, err)) {
		MessageBoxW(hwnd, Utf8ToWide(err).c_str(), L"Error", MB_ICONERROR);
		return;
	}
	std::string hex = hashcore::to_hex(digest, g_state.uppercaseHex);
	std::string b64 = hashcore::to_base64(digest);
	double mb = (double)sizeBytes / (1024.0 * 1024.0);
	double thr = elapsed > 0.0 ? (mb / elapsed) : 0.0;

	g_state.hexOut = Utf8ToWide(hex);
	g_state.b64Out = Utf8ToWide(b64);
	g_state.sizeOut = std::to_wstring(sizeBytes) + L" bytes";
	{
		long long ms = (long long)(elapsed * 1000.0 + 0.5);
		g_state.elapsedOut = std::to_wstring(ms) + L" ms";
	}
	{
		std::wstringstream ss; ss << thr; g_state.throughputOut = ss.str() + L" MiB/s";
	}

	SetControlText(hwnd, 101, g_state.hexOut);
	SetControlText(hwnd, 102, g_state.b64Out);
	SetControlText(hwnd, 103, g_state.sizeOut);
	SetControlText(hwnd, 104, g_state.elapsedOut);
	SetControlText(hwnd, 105, g_state.throughputOut);
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
		DoHash(hwnd);
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
			if (!g_state.hexOut.empty()) DoHash(hwnd);
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
		if (DragQueryFileW(hDrop, 0, path, MAX_PATH)) {
			g_state.selectedPath = path;
			SetControlText(hwnd, 100, g_state.selectedPath);
			DoHash(hwnd);
		}
		DragFinish(hDrop);
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

	HWND hwnd = CreateWindowExW(0, kWndClass, L"c-hash (GUI)", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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


