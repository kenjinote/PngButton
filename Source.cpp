#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"gdiplus")

#include <windows.h>
#include <gdiplus.h>
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

Gdiplus::Bitmap* loadpng(int nID)
{
	Gdiplus::Bitmap* pImage = 0;
	const HMODULE hInst = GetModuleHandle(0);
	const HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(nID), TEXT("PNG"));
	if (!hResource)
		return 0;
	const DWORD dwImageSize = SizeofResource(hInst, hResource);
	if (!dwImageSize)
		return 0;
	const void* pResourceData = LockResource(LoadResource(hInst, hResource));
	if (!pResourceData)
		return 0;
	const HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwImageSize);
	if (hBuffer)
	{
		void* pBuffer = GlobalLock(hBuffer);
		if (pBuffer)
		{
			CopyMemory(pBuffer, pResourceData, dwImageSize);
			IStream* pStream = NULL;
			if (CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
			{
				pImage = Gdiplus::Bitmap::FromStream(pStream);
				pStream->Release();
				if (pImage)
				{
					if (pImage->GetLastStatus() != Gdiplus::Ok)
					{
						delete pImage;
						pImage = NULL;
					}
				}
			}
			::GlobalUnlock(hBuffer);
		}
	}
	return pImage;
}

class CBitmapButton
{
public:
	CBitmapButton() :
		m_bHover(0),
		m_bPush(0),
		m_bDisable(0),
		m_pNormal(0),
		m_pPush(0),
		m_pHover(0),
		m_pDisable(0) {}
	~CBitmapButton() {
		DestroyWindow(m_hWnd);
		delete m_pNormal;
		delete m_pPush;
		delete m_pHover;
		delete m_pDisable;
	}
	HWND CreateButton(
		int x,               // ウィンドウの横方向の位置
		int y,               // ウィンドウの縦方向の位置
		int nWidth,          // ウィンドウの幅
		int nHeight,         // ウィンドウの高さ
		HWND hWndParent,     // 親ウィンドウまたはオーナーウィンドウのハンドル
		HMENU hMenu,         // メニューハンドルまたは子ウィンドウ ID
		HINSTANCE hInstance, // アプリケーションインスタンスのハンドル
		int nNorm,           // ノーマルのときのリソースビットマップID
		int nSel,            // 押したときのリソースビットマップID
		int nHover,          // ホバーのときのリソースビットマップID
		int nDis             // ディセーブルのときのリソースビットマップID
	) {
		m_pNormal = loadpng(nNorm);
		m_pPush = loadpng(nSel);
		m_pHover = loadpng(nHover);
		m_pDisable = loadpng(nDis);
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_hWnd = CreateWindow(TEXT("BUTTON"), 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, 0);
		if (!m_hWnd) return NULL;
		SetClassLong(m_hWnd, GCL_STYLE, GetClassLong(m_hWnd, GCL_STYLE) & ~CS_DBLCLKS);
		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this); // ウィンドウにインスタンスのポインタを保持しておく
		m_DefBtnProc = (WNDPROC)SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)GlobalButtonProc);
		return m_hWnd;
	}
private:
	HWND m_hWnd;
	int m_nWidth;			// ボタンの幅
	int m_nHeight;			// ボタンの高さ
	Gdiplus::Bitmap* m_pNormal;
	Gdiplus::Bitmap* m_pPush;
	Gdiplus::Bitmap* m_pHover;
	Gdiplus::Bitmap* m_pDisable;
	WNDPROC m_DefBtnProc;	// ボタンのデフォルトウィンドウプロシージャを保持
	BOOL m_bHover;			// ホーバー状態かどうかのフラグ
	BOOL m_bPush;			// プッシュ状態かどうかのフラグ
	BOOL m_bDisable;		// 無効状態(ディセーブル）かどうかのフラグ
	virtual LRESULT CBitmapButton::LocalButtonProc(
		HWND hWnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	) {
		switch (msg) {
		case WM_PRINTCLIENT: // これを処理しないとAnimateWindow()が動作しない
		case WM_ERASEBKGND: // 背景を描画
		{
			const HDC hMemDC = CreateCompatibleDC((HDC)wParam);
			HBITMAP hBitmap = 0;
			if (m_bDisable) {
				m_pDisable->GetHBITMAP(0, &hBitmap);
			}
			else if (m_bPush) {
				m_pPush->GetHBITMAP(0, &hBitmap);
			}
			else {
				if (m_bHover) {
					m_pHover->GetHBITMAP(0, &hBitmap);
				}
				else {
					m_pNormal->GetHBITMAP(0, &hBitmap);
				}
			}
			if (hBitmap) {
				const HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
				BitBlt((HDC)wParam, 0, 0, m_nWidth, m_nHeight, hMemDC, 0, 0, SRCCOPY);
				SelectObject(hMemDC, hOldBitmap);
				DeleteObject(hBitmap);
				DeleteDC(hMemDC);
			}
		}
		return 1;
		case WM_ENABLE: // 無効・有効状態の切り分け
			m_bDisable = !(BOOL)wParam;
			InvalidateRect(hWnd, 0, TRUE);
			break;
		case WM_LBUTTONDOWN:
			m_bPush = TRUE;
			m_bHover = FALSE;
			InvalidateRect(hWnd, 0, TRUE);
			break;
		case WM_LBUTTONUP:
			m_bPush = FALSE;
			InvalidateRect(hWnd, 0, TRUE);
			break;
		case WM_MOUSEMOVE:
			if (!m_bPush && !m_bHover) {
				m_bHover = TRUE;
				TRACKMOUSEEVENT	tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				TrackMouseEvent(&tme);
				InvalidateRect(hWnd, 0, TRUE);
			}
			break;
		case WM_MOUSELEAVE:
			m_bHover = FALSE;
			InvalidateRect(hWnd, 0, TRUE);
			break;
		case WM_DESTROY:
			m_hWnd = NULL;
			break;
		}
		return CallWindowProc(m_DefBtnProc, hWnd, msg, wParam, lParam);
	}
	static LRESULT CALLBACK CBitmapButton::GlobalButtonProc(
		HWND hWnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	) {
		CBitmapButton *p = (CBitmapButton*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (p) {
			return p->LocalButtonProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CBitmapButton* pBtn;
	switch (msg) {
	case WM_CREATE:
		pBtn = new CBitmapButton();
		pBtn->CreateButton(10, 10, 120, 34, hWnd, (HMENU)IDOK,
			((LPCREATESTRUCT)(lParam))->hInstance,
			IDB_PNG1, IDB_PNG2, IDB_PNG3, IDB_PNG4);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			MessageBox(hWnd, TEXT("ボタンが押されました。"), 0, 0);
		}
		break;
	case WM_DESTROY:
		delete pBtn;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	// GDI+の初期化
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	WNDCLASS wndclass = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, 0,
		LoadCursor(0,IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), 0, szClassName };
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(szClassName, TEXT("Window"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Gdiplus::GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}
