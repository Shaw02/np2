/**
 * @file	trace.cpp
 * @brief	トレース クラスの動作の定義を行います
 */

#include "compiler.h"
#include <stdarg.h>
#include "resource.h"
#include "misc\WndProc.h"
#include "strres.h"
#include "textfile.h"
#include "dosio.h"
#include "ini.h"
#include "menu.h"

#ifdef TRACE

#define	VIEW_BUFFERSIZE	4096
#define	VIEW_FGCOLOR	0x000000
#define	VIEW_BGCOLOR	0xffffff
#define	VIEW_TEXT		"ＭＳ ゴシック"
#define	VIEW_SIZE		12

/**
 * @brief トレース ウィンドウ クラス
 */
class CTraceWnd : public CWndProc
{
public:
	static CTraceWnd* GetInstance();

	CTraceWnd();
	void Initialize();
	void Deinitialize();
	bool IsTrace() const;
	bool IsVerbose() const;
	bool IsEnabled() const;
	void AddString(LPCTSTR lpString);

protected:
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnEnterMenuLoop(BOOL bIsTrackPopupMenu);

private:
	static CTraceWnd sm_instance;		/*!< 唯一のインスタンスです */
	UINT8 m_nFlags;						/*!< フラグ */
	TEXTFILEH m_tfh;					/*!< テキスト ファイル ハンドル */
	HBRUSH m_hBrush;					/*!< ブラシ */
	HFONT m_hFont;						/*!< フォント */
	CWndProc m_wndView;					/*!< テキスト コントロール */

private:
	int viewpos;
	int viewleng;
	TCHAR viewbuf[VIEW_BUFFERSIZE * 2];
	void View_ScrollToBottom();
	void View_ClrString();
	void View_AddString(const OEMCHAR *string);
};

struct TRACECFG
{
	int		posx;
	int		posy;
	int		width;
	int		height;
};

static const TCHAR s_szTitle[] = TEXT("console");
static const TCHAR s_szClassName[] = TEXT("TRACE-console");
static const TCHAR s_szViewFont[] = TEXT(VIEW_TEXT);

static const OEMCHAR crlf[] = OEMTEXT("\r\n");

static	TRACECFG	tracecfg;
static	int			devpos;
static	char		devstr[256];

static const OEMCHAR np2trace[] = OEMTEXT("np2trace.ini");
static const OEMCHAR inititle[] = OEMTEXT("TRACE");
static const PFTBL initbl[4] =
{
	PFVAL("posx",	PFTYPE_SINT32,	&tracecfg.posx),
	PFVAL("posy",	PFTYPE_SINT32,	&tracecfg.posy),
	PFVAL("width",	PFTYPE_SINT32,	&tracecfg.width),
	PFVAL("height",	PFTYPE_SINT32,	&tracecfg.height)
};


// ---- View

void CTraceWnd::View_ScrollToBottom()
{
	int MinPos;
	int MaxPos;

	GetScrollRange(m_wndView, SB_VERT, &MinPos, &MaxPos);
	m_wndView.PostMessage(EM_LINESCROLL, 0, MaxPos);
}

void CTraceWnd::View_ClrString(void)
{
	viewpos = 0;
	viewleng = 0;
	viewbuf[0] = '\0';
	m_wndView.SetWindowText(viewbuf);
}

void CTraceWnd::View_AddString(const OEMCHAR *string)
{
	int		slen;
	int		vpos;
	int		vlen;
	TCHAR	c;

	slen = lstrlen(string);
	if ((slen == 0) || ((slen + 3) > VIEW_BUFFERSIZE)) {
		return;
	}
	vpos = viewpos;
	vlen = viewleng;
	if ((vpos + vlen + slen + 3) > (VIEW_BUFFERSIZE * 2)) {
		while(vlen > 0) {
			vlen--;
			c = viewbuf[vpos++];
			if ((c == 0x0a) && ((vlen + slen + 3) <= VIEW_BUFFERSIZE)) {
				break;
			}
		}
		if (vpos >= VIEW_BUFFERSIZE) {
			if (vlen) {
				CopyMemory(viewbuf, viewbuf + vpos, vlen * sizeof(TCHAR));
			}
			vpos = 0;
			viewpos = 0;
		}
	}
	CopyMemory(viewbuf + vpos + vlen, string, slen * sizeof(TCHAR));
	vlen += slen;
	viewbuf[vpos + vlen + 0] = '\r';
	viewbuf[vpos + vlen + 1] = '\n';
	viewbuf[vpos + vlen + 2] = '\0';
	viewleng = vlen + 2;
	m_wndView.SetWindowText(viewbuf + vpos);
	View_ScrollToBottom();
}


// ----

CTraceWnd CTraceWnd::sm_instance;

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline CTraceWnd* CTraceWnd::GetInstance()
{
	return &sm_instance;
}

/**
 * コンストラクタ
 */
CTraceWnd::CTraceWnd()
	: m_nFlags(0)
	, m_tfh(NULL)
	, m_hBrush(NULL)
	, m_hFont(NULL)
{
}

/**
 * Trace は有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CTraceWnd::IsTrace() const
{
	return ((m_nFlags & 1) != 0);
}

/**
 * Verbose は有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CTraceWnd::IsVerbose() const
{
	return ((m_nFlags & 2) != 0);
}

/**
 * 有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CTraceWnd::IsEnabled() const
{
	return ((m_nFlags & 4) || (m_tfh != NULL));
}

/**
 * ログ追加
 * @param[in] lpString 文字列
 */
void CTraceWnd::AddString(LPCTSTR lpString)
{
	if ((m_nFlags & 4) && (m_wndView.IsWindow()))
	{
		View_AddString(lpString);
	}
	if (m_tfh != NULL)
	{
		textfile_write(m_tfh, lpString);
		textfile_write(m_tfh, crlf);
	}
}

/**
 * フレームワークは、Windows のウィンドウは [作成] または CreateEx のメンバー関数を呼び出すことによって作成されたアプリケーションが必要とすると、このメンバー関数を呼び出します
 * @param[in] lpCreateStruct 作成されたオブジェクトに関する情報が含まれています。
 * @retval 0 成功
 * @retval -1 失敗
 */
int CTraceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	HMENU hMenu = GetSystemMenu(FALSE);
	menu_addmenures(hMenu, 0, IDR_TRACE, FALSE);

	m_hBrush = ::CreateSolidBrush(VIEW_BGCOLOR);

	RECT rc;
	GetClientRect(&rc);
	if (m_wndView.CreateEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | ES_READONLY | ES_LEFT | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL, 0, 0, rc.right, rc.bottom, m_hWnd, NULL))
	{
		m_wndView.SendMessage(EM_SETLIMITTEXT, VIEW_BUFFERSIZE, 0);

		m_hFont = ::CreateFont(VIEW_SIZE, 0, 0, 0, 0, 0, 0, 0,  SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, s_szViewFont);
		if (m_hFont)
		{
			m_wndView.SetFont(m_hFont);
		}
		m_wndView.SetFocus();
	}

	return 0;
}

/**
 * フレームワークは、ユーザーがコントロール メニューからコマンドを選択したとき、または最大化または最小化ボタンを選択すると、このメンバー関数を呼び出します
 * @param[in] nID 必要なシステム コマンドの種類を指定します
 * @param[in] lParam カーソルの座標
 */
void CTraceWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch (nID)
	{
		case IDM_TRACE_TRACE:
			m_nFlags ^= 1;
			break;

		case IDM_TRACE_VERBOSE:
			m_nFlags ^= 2;
			break;

		case IDM_TRACE_ENABLE:
			m_nFlags ^= 4;
			break;

		case IDM_TRACE_FILEOUT:
			if (m_tfh != NULL)
			{
				textfile_close(m_tfh);
				m_tfh = NULL;
			}
			else
			{
				m_tfh = textfile_create(OEMTEXT("traceout.txt"), 0x800);
			}
			break;

		case IDM_TRACE_CLEAR:
			View_ClrString();
			break;
	}
}

/**
 * Windows プロシージャ (WindowProc) を提供します
 * @param[in] message 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理に使用する追加情報を提供します
 * @param[in] lParam メッセージの処理に使用する追加情報を提供します
 * @return 戻り値は、メッセージによって異なります
 */
LRESULT CTraceWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_CREATE:
			return OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam));

		case WM_SYSCOMMAND:
			OnSysCommand(wParam, lParam);
			return DefWindowProc(nMsg, wParam, lParam);

		case WM_ENTERMENULOOP:
			OnEnterMenuLoop(wParam);
			break;

		case WM_MOVE:
			if (!(GetStyle() & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				tracecfg.posx = rc.left;
				tracecfg.posy = rc.top;
			}
			break;

		case WM_SIZE:							// window resize
			if (!(GetStyle() & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				tracecfg.width = rc.right - rc.left;
				tracecfg.height = rc.bottom - rc.top;
			}
			m_wndView.MoveWindow(0, 0, LOWORD(lParam), HIWORD(lParam));
			View_ScrollToBottom();
			break;

		case WM_SETFOCUS:
			m_wndView.SetFocus();
			break;

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
			SetTextColor((HDC)wParam, VIEW_FGCOLOR);
			SetBkColor((HDC)wParam, VIEW_BGCOLOR);
			return reinterpret_cast<LRESULT>(m_hBrush);

		case WM_CLOSE:
			break;

		case WM_DESTROY:
			if (m_hBrush)
			{
				::DeleteObject(m_hBrush);
				m_hBrush = NULL;
			}
			if (m_hFont)
			{
				::DeleteObject(m_hFont);
				m_hFont = NULL;
			}
			break;
#if 0
		case WM_ENTERSIZEMOVE:
			winloc_movingstart();
			break;

		case WM_MOVING:
			winloc_movingproc((RECT *)lParam);
			break;

		case WM_ERASEBKGND:
			break;
#endif
		default:
			return DefWindowProc(nMsg, wParam, lParam);
	}
	return FALSE;
}

/**
 * フレームワークは、メニュー ループ開始時に、このメンバー関数を呼び出します
 * @param[in] bIsTrackPopupMenu TrackPopupMenu 関数を利用した場合 TRUE
 */
void CTraceWnd::OnEnterMenuLoop(BOOL bIsTrackPopupMenu)
{
	HMENU hMenu = GetSystemMenu(FALSE);
	::CheckMenuItem(hMenu, IDM_TRACE_TRACE, (m_nFlags & 1) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, IDM_TRACE_VERBOSE, (m_nFlags & 2) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, IDM_TRACE_ENABLE, (m_nFlags & 4) ? MF_CHECKED:MF_UNCHECKED);
	::CheckMenuItem(hMenu, IDM_TRACE_FILEOUT, (m_tfh != NULL) ? MF_CHECKED : MF_UNCHECKED);
}



// ----

void trace_init(void)
{
	CTraceWnd::GetInstance()->Initialize();
}

/**
 * 初期化
 */
void CTraceWnd::Initialize()
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = GetInstanceHandle();
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
	wc.lpszClassName = s_szClassName;
	if (!::RegisterClass(&wc))
	{
		return;
	}

#if 1
	m_nFlags = 4;
#else
	m_nFlags = 1;
	m_tfh = textfile_create(OEMTEXT("traceout.txt"), 0x800);
#endif

	tracecfg.posx = CW_USEDEFAULT;
	tracecfg.posy = CW_USEDEFAULT;
	tracecfg.width = CW_USEDEFAULT;
	tracecfg.height = CW_USEDEFAULT;
	ini_read(file_getcd(np2trace), inititle, initbl, NELEMENTS(initbl));

	if (!CreateEx(WS_EX_CONTROLPARENT, s_szClassName, s_szTitle, WS_OVERLAPPEDWINDOW, tracecfg.posx, tracecfg.posy, tracecfg.width, tracecfg.height, NULL, NULL))
	{
		return;
	}
	ShowWindow(SW_SHOW);
	UpdateWindow();
}

void trace_term(void)
{
	CTraceWnd::GetInstance()->Deinitialize();
}

/**
 * 解放
 */
void CTraceWnd::Deinitialize()
{
	if (m_tfh != NULL)
	{
		textfile_close(m_tfh);
		m_tfh = NULL;
	}

	DestroyWindow();
	ini_write(file_getcd(np2trace), inititle, initbl, NELEMENTS(initbl));
}

void trace_fmt(const char *fmt, ...)
{
	CTraceWnd* pWnd = CTraceWnd::GetInstance();

	if ((pWnd->IsTrace()) && (pWnd->IsEnabled()))
	{
		va_list ap;
		va_start(ap, fmt);
#if defined(OSLANG_UCS2)
		OEMCHAR cnvfmt[0x800];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fmt, -1, cnvfmt, NELEMENTS(cnvfmt));
		OEMCHAR buf[0x1000];
		vswprintf(buf, cnvfmt, ap);
#else
		OEMCHAR buf[0x1000];
		vsprintf(buf, fmt, ap);
#endif
		va_end(ap);
		pWnd->AddString(buf);
	}
}

void trace_fmt2(const char *fmt, ...)
{
	CTraceWnd* pWnd = CTraceWnd::GetInstance();

	if ((pWnd->IsVerbose()) && (pWnd->IsEnabled()))
	{
		va_list ap;
		va_start(ap, fmt);
#if defined(OSLANG_UCS2)
		OEMCHAR cnvfmt[0x800];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fmt, -1, cnvfmt, NELEMENTS(cnvfmt));

		OEMCHAR buf[0x1000];
		vswprintf(buf, cnvfmt, ap);
#else
		OEMCHAR buf[0x1000];
		vsprintf(buf, fmt, ap);
#endif
		va_end(ap);
		pWnd->AddString(buf);
	}
}

void trace_char(char c)
{
	if ((c == 0x0a) || (c == 0x0d))
	{
		if (devpos)
		{
			devstr[devpos] = '\0';
#if defined(OSLANG_UCS2)
			TCHAR pdevstr[0x800];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, devstr, -1, pdevstr, NELEMENTS(pdevstr));
#else
			const OEMCHAR *pdevstr = devstr;
#endif
			CTraceWnd::GetInstance()->AddString(pdevstr);
			devpos = 0;
		}
	}
	else
	{
		if (devpos < (sizeof(devstr) - 1))
		{
			devstr[devpos++] = c;
		}
	}
}

#endif
