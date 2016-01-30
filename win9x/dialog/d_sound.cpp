/**
 * @file	d_sound.cpp
 * @brief	Sound configure dialog procedure
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include <vector>
#include "c_combodata.h"
#include "c_dipsw.h"
#include "c_slidervalue.h"
#include "dialogs.h"
#include "np2class.h"
#include "dosio.h"
#include "joymng.h"
#include "np2.h"
#include "sysmng.h"
#include "misc\PropProc.h"
#include "misc\tstring.h"
#include "pccore.h"
#include "iocore.h"
#include "common\strres.h"
#include "generic\dipswbmp.h"
#include "sound\sound.h"
#include "sound\fmboard.h"
#include "sound\tms3631.h"

// ---- mixer

/**
 * @brief Mixer ページ
 */
class SndOptMixerPage : public CPropPageProc
{
public:
	SndOptMixerPage();
	virtual ~SndOptMixerPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	CSliderValue m_fm;			//!< FM ヴォリューム
	CSliderValue m_psg;			//!< PSG ヴォリューム
	CSliderValue m_adpcm;		//!< ADPCM ヴォリューム
	CSliderValue m_pcm;			//!< PCM ヴォリューム
	CSliderValue m_rhythm;		//!< RHYTHM ヴォリューム
};

/**
 * コンストラクタ
 */
SndOptMixerPage::SndOptMixerPage()
	: CPropPageProc(IDD_SNDMIX)
{
}

/**
 * デストラクタ
 */
SndOptMixerPage::~SndOptMixerPage()
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SndOptMixerPage::OnInitDialog()
{
	m_fm.SubclassDlgItem(IDC_VOLFM, this);
	m_fm.SetStaticId(IDC_VOLFMSTR);
	m_fm.SetRange(0, 128);
	m_fm.SetPos(np2cfg.vol_fm);

	m_psg.SubclassDlgItem(IDC_VOLPSG, this);
	m_psg.SetStaticId(IDC_VOLPSGSTR);
	m_psg.SetRange(0, 128);
	m_psg.SetPos(np2cfg.vol_ssg);

	m_adpcm.SubclassDlgItem(IDC_VOLADPCM, this);
	m_adpcm.SetStaticId(IDC_VOLADPCMSTR);
	m_adpcm.SetRange(0, 128);
	m_adpcm.SetPos(np2cfg.vol_adpcm);

	m_pcm.SubclassDlgItem(IDC_VOLPCM, this);
	m_pcm.SetStaticId(IDC_VOLPCMSTR);
	m_pcm.SetRange(0, 128);
	m_pcm.SetPos(np2cfg.vol_pcm);

	m_rhythm.SubclassDlgItem(IDC_VOLRHYTHM, this);
	m_rhythm.SetStaticId(IDC_VOLRHYTHMSTR);
	m_rhythm.SetRange(0, 128);
	m_rhythm.SetPos(np2cfg.vol_rhythm);

	return TRUE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SndOptMixerPage::OnOK()
{
	bool bUpdated = false;

	const UINT8 cFM = static_cast<UINT8>(m_fm.GetPos());
	if (np2cfg.vol_fm != cFM)
	{
		np2cfg.vol_fm = cFM;
		opngen_setvol(cFM);
		bUpdated = true;
	}

	const UINT8 cPSG = static_cast<UINT8>(m_psg.GetPos());
	if (np2cfg.vol_ssg != cPSG)
	{
		np2cfg.vol_ssg = cPSG;
		psggen_setvol(cPSG);
		bUpdated = true;
	}

	const UINT8 cADPCM = static_cast<UINT8>(m_adpcm.GetPos());
	if (np2cfg.vol_adpcm != cADPCM)
	{
		np2cfg.vol_adpcm = cADPCM;
		adpcm_setvol(cADPCM);
		for (UINT i = 0; i < _countof(g_opna); i++)
		{
			adpcm_update(&g_opna[i].adpcm);
		}
		bUpdated = true;
	}

	const UINT8 cPCM = static_cast<UINT8>(m_pcm.GetPos());
	if (np2cfg.vol_pcm != cPCM)
	{
		np2cfg.vol_pcm = cPCM;
		pcm86gen_setvol(cPCM);
		pcm86gen_update();
		bUpdated = true;
	}

	const UINT8 cRhythm = static_cast<UINT8>(m_rhythm.GetPos());
	if (np2cfg.vol_rhythm != cRhythm)
	{
		np2cfg.vol_rhythm = cRhythm;
		rhythm_setvol(cRhythm);
		for (UINT i = 0; i < _countof(g_opna); i++)
		{
			rhythm_update(&g_opna[i].rhythm);
		}
		bUpdated = true;
	}

	if (bUpdated)
	{
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL SndOptMixerPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_SNDMIXDEF)
	{
		m_fm.SetPos(64);
		m_psg.SetPos(64);
		m_adpcm.SetPos(64);
		m_pcm.SetPos(64);
		m_rhythm.SetPos(64);
		return TRUE;
	}
	return FALSE;
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT SndOptMixerPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_HSCROLL)
	{
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)))
		{
			case IDC_VOLFM:
				m_fm.UpdateValue();
				break;

			case IDC_VOLPSG:
				m_psg.UpdateValue();
				break;

			case IDC_VOLADPCM:
				m_adpcm.UpdateValue();
				break;

			case IDC_VOLPCM:
				m_pcm.UpdateValue();
				break;

			case IDC_VOLRHYTHM:
				m_rhythm.UpdateValue();
				break;

			default:
				break;
		}
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}



// ---- PC-9801-14

/**
 * @brief 14 ページ
 */
class SndOpt14Page : public CPropPageProc
{
public:
	SndOpt14Page();
	virtual ~SndOpt14Page();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	/**
	 * @brief アイテム
	 */
	struct Item
	{
		UINT nSlider;		//!< スライダー
		UINT nStatic;		//!< スタティック
	};

	CSliderValue m_vol[6];	//!< ヴォリューム
};

/**
 * コンストラクタ
 */
SndOpt14Page::SndOpt14Page()
	: CPropPageProc(IDD_SND14)
{
}

/**
 * デストラクタ
 */
SndOpt14Page::~SndOpt14Page()
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SndOpt14Page::OnInitDialog()
{
	static const Item s_snd14item[6] =
	{
		{IDC_VOL14L,	IDC_VOL14LSTR},
		{IDC_VOL14R,	IDC_VOL14RSTR},
		{IDC_VOLF2,		IDC_VOLF2STR},
		{IDC_VOLF4,		IDC_VOLF4STR},
		{IDC_VOLF8,		IDC_VOLF8STR},
		{IDC_VOLF16,	IDC_VOLF16STR},
	};

	for (UINT i = 0; i < 6; i++)
	{
		m_vol[i].SubclassDlgItem(s_snd14item[i].nSlider, this);
		m_vol[i].SetStaticId(s_snd14item[i].nStatic);
		m_vol[i].SetRange(0, 15);
		m_vol[i].SetPos(np2cfg.vol14[i]);
	}

	return TRUE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SndOpt14Page::OnOK()
{
	bool bUpdated = false;

	for (UINT i = 0; i < 6; i++)
	{
		const UINT8 cVol = static_cast<UINT8>(m_vol[i].GetPos());
		if (np2cfg.vol14[i] != cVol)
		{
			np2cfg.vol14[i] = cVol;
			bUpdated = true;
		}
	}

	if (bUpdated)
	{
		::tms3631_setvol(np2cfg.vol14);
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT SndOpt14Page::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_HSCROLL)
	{
		for (UINT i = 0; i < 6; i++)
		{
			if (m_vol[i] == reinterpret_cast<HWND>(lParam))
			{
				m_vol[i].UpdateValue();
				break;
			}
		}
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}



// ---- 26K, SPB jumper

static const CBPARAM cpIO26[] =
{
	{MAKEINTRESOURCE(IDS_0088),		0x00},
	{MAKEINTRESOURCE(IDS_0188),		0x10},
};

static const CBPARAM cpInt26[] =
{
	{MAKEINTRESOURCE(IDS_INT0),		0x00},
	{MAKEINTRESOURCE(IDS_INT41),	0x80},
	{MAKEINTRESOURCE(IDS_INT5),		0xc0},
	{MAKEINTRESOURCE(IDS_INT6),		0x40},
};

static const CBPARAM cpAddr[] =
{
	{MAKEINTRESOURCE(IDS_C8000),		0x00},
	{MAKEINTRESOURCE(IDS_CC000),		0x01},
	{MAKEINTRESOURCE(IDS_D0000),		0x02},
	{MAKEINTRESOURCE(IDS_D4000),		0x03},
	{MAKEINTRESOURCE(IDS_NONCONNECT),	0x04},
};

static void setsnd26io(HWND hWnd, UINT uID, UINT8 cValue)
{
	dlgs_setcbcur(hWnd, uID, cValue & 0x10);
}

static UINT8 getsnd26io(HWND hWnd, UINT uID)
{
	return dlgs_getcbcur(hWnd, uID, 0x10);
}

static void setsnd26int(HWND hWnd, UINT uID, UINT8 cValue)
{
	dlgs_setcbcur(hWnd, uID, cValue & 0xc0);
}

static UINT8 getsnd26int(HWND hWnd, UINT uID)
{
	return dlgs_getcbcur(hWnd, uID, 0xc0);
}

static void setsnd26rom(HWND hWnd, UINT uID, UINT8 cValue)
{
	UINT	uParam;

	uParam = cValue & 0x07;
	uParam = min(uParam, 0x04);
	dlgs_setcbcur(hWnd, uID, uParam);
}

static UINT8 getsnd26rom(HWND hWnd, UINT uID)
{
	return dlgs_getcbcur(hWnd, uID, 0x04);
}



// ---- PC-9801-26

static	UINT8	snd26 = 0;

static void snd26set(HWND hWnd, UINT8 cValue)
{
	setsnd26io(hWnd, IDC_SND26IO, cValue);
	setsnd26int(hWnd, IDC_SND26INT, cValue);
	setsnd26rom(hWnd, IDC_SND26ROM, cValue);
}

static void set26jmp(HWND hWnd, UINT8 value, UINT8 bit) {

	if ((snd26 ^ value) & bit) {
		snd26 &= ~bit;
		snd26 |= value;
		InvalidateRect(GetDlgItem(hWnd, IDC_SND26JMP), NULL, TRUE);
	}
}

static void snd26cmdjmp(HWND hWnd) {

	RECT	rect1;
	RECT	rect2;
	POINT	p;
	BOOL	redraw;
	UINT8	b;
	UINT8	bit;

	GetWindowRect(GetDlgItem(hWnd, IDC_SND26JMP), &rect1);
	GetClientRect(GetDlgItem(hWnd, IDC_SND26JMP), &rect2);
	GetCursorPos(&p);
	redraw = FALSE;
	p.x += rect2.left - rect1.left;
	p.y += rect2.top - rect1.top;
	p.x /= 9;
	p.y /= 9;
	if ((p.y < 1) || (p.y >= 3)) {
		return;
	}
	if ((p.x >= 2) && (p.x < 7)) {
		b = (UINT8)(p.x - 2);
		if ((snd26 ^ b) & 7) {
			snd26 &= ~0x07;
			snd26 |= b;
			setsnd26rom(hWnd, IDC_SND26ROM, b);
			redraw = TRUE;
		}
	}
	else if ((p.x >= 9) && (p.x < 12)) {
		b = snd26;
		bit = 0x40 << (2 - p.y);
		switch(p.x) {
			case 9:
				b |= bit;
				break;

			case 10:
				b ^= bit;
				break;

			case 11:
				b &= ~bit;
				break;
		}
		if (snd26 != b) {
			snd26 = b;
			setsnd26int(hWnd, IDC_SND26INT, b);
			redraw = TRUE;
		}
	}
	else if ((p.x >= 15) && (p.x < 17)) {
		b = (UINT8)((p.x - 15) << 4);
		if ((snd26 ^ b) & 0x10) {
			snd26 &= ~0x10;
			snd26 |= b;
			setsnd26io(hWnd, IDC_SND26IO, b);
			redraw = TRUE;
		}
	}
	if (redraw) {
		InvalidateRect(GetDlgItem(hWnd, IDC_SND26JMP), NULL, TRUE);
	}
}

static LRESULT CALLBACK Snd26optDlgProc(HWND hWnd, UINT msg,
													WPARAM wp, LPARAM lp) {

	HWND	sub;

	switch(msg) {
		case WM_INITDIALOG:
			snd26 = np2cfg.snd26opt;
			dlgs_setcbitem(hWnd, IDC_SND26IO, cpIO26, NELEMENTS(cpIO26));
			dlgs_setcbitem(hWnd, IDC_SND26INT, cpInt26, NELEMENTS(cpInt26));
			dlgs_setcbitem(hWnd, IDC_SND26ROM, cpAddr, NELEMENTS(cpAddr));
			snd26set(hWnd, snd26);
			sub = GetDlgItem(hWnd, IDC_SND26JMP);
			SetWindowLong(sub, GWL_STYLE, SS_OWNERDRAW +
							(GetWindowLong(sub, GWL_STYLE) & (~SS_TYPEMASK)));
			return(TRUE);

		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case IDC_SND26IO:
					set26jmp(hWnd, getsnd26io(hWnd, IDC_SND26IO), 0x10);
					break;

				case IDC_SND26INT:
					set26jmp(hWnd, getsnd26int(hWnd, IDC_SND26INT), 0xc0);
					break;

				case IDC_SND26ROM:
					set26jmp(hWnd, getsnd26rom(hWnd, IDC_SND26ROM), 0x07);
					break;

				case IDC_SND26DEF:
					snd26 = 0xd1;
					snd26set(hWnd, snd26);
					InvalidateRect(GetDlgItem(hWnd, IDC_SND26JMP), NULL, TRUE);
					break;

				case IDC_SND26JMP:
					snd26cmdjmp(hWnd);
					break;
			}
			break;

		case WM_NOTIFY:
			if ((((NMHDR *)lp)->code) == (UINT)PSN_APPLY) {
				if (np2cfg.snd26opt != snd26) {
					np2cfg.snd26opt = snd26;
					sysmng_update(SYS_UPDATECFG);
				}
				return(TRUE);
			}
			break;

		case WM_DRAWITEM:
			if (LOWORD(wp) == IDC_SND26JMP) {
				dlgs_drawbmp(((LPDRAWITEMSTRUCT)lp)->hDC,
												dipswbmp_getsnd26(snd26));
			}
			break;
	}
	return(FALSE);
}



// ---- PC-9801-86

/**
 * @brief 86 ページ
 */
class SndOpt86Page : public CPropPageProc
{
public:
	SndOpt86Page();
	virtual ~SndOpt86Page();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_snd86;				//!< 設定値
	CComboData m_io;			//!< IO
	CComboData m_int;			//!< INT
	CComboData m_id;			//!< ID
	CStaticDipSw m_dipsw;		//!< DIPSW
	void Set(UINT8 cValue);
	void SetJumper(UINT cAdd, UINT cRemove);
	void OnDipSw();
};

//! 86 I/O
static const CComboData::Entry s_io86[] =
{
	{MAKEINTRESOURCE(IDS_0188),		0x01},
	{MAKEINTRESOURCE(IDS_0288),		0x00},
};

//! 86 INT
static const CComboData::Entry s_int86[] =
{
	{MAKEINTRESOURCE(IDS_INT0),		0x00},
	{MAKEINTRESOURCE(IDS_INT41),	0x08},
	{MAKEINTRESOURCE(IDS_INT5),		0x0c},
	{MAKEINTRESOURCE(IDS_INT6),		0x04},
};

//! 86 ID
static const CComboData::Entry s_id86[] =
{
	{MAKEINTRESOURCE(IDS_0X),	0xe0},
	{MAKEINTRESOURCE(IDS_1X),	0xc0},
	{MAKEINTRESOURCE(IDS_2X),	0xa0},
	{MAKEINTRESOURCE(IDS_3X),	0x80},
	{MAKEINTRESOURCE(IDS_4X),	0x60},
	{MAKEINTRESOURCE(IDS_5X),	0x40},
	{MAKEINTRESOURCE(IDS_6X),	0x20},
	{MAKEINTRESOURCE(IDS_7X),	0x00},
};

/**
 * コンストラクタ
 */
SndOpt86Page::SndOpt86Page()
	: CPropPageProc(IDD_SND86)
	, m_snd86(0)
{
}

/**
 * デストラクタ
 */
SndOpt86Page::~SndOpt86Page()
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SndOpt86Page::OnInitDialog()
{
	m_io.SubclassDlgItem(IDC_SND86IO, this);
	m_io.Add(s_io86, _countof(s_io86));

	m_int.SubclassDlgItem(IDC_SND86INTA, this);
	m_int.Add(s_int86, _countof(s_int86));

	m_id.SubclassDlgItem(IDC_SND86ID, this);
	m_id.Add(s_id86, _countof(s_id86));

	Set(np2cfg.snd86opt);

	m_dipsw.SubclassDlgItem(IDC_SND86DIP, this);

	m_io.SetFocus();
	return FALSE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SndOpt86Page::OnOK()
{
	if (np2cfg.snd86opt != m_snd86)
	{
		np2cfg.snd86opt = m_snd86;
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL SndOpt86Page::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_SND86IO:
			SetJumper(m_io.GetCurItemData(m_snd86 & 0x01), 0x01);
			break;

		case IDC_SND86INT:
			SetJumper((IsDlgButtonChecked(IDC_SND86INT) != BST_UNCHECKED) ?0x10 : 0x00, 0x10);
			break;

		case IDC_SND86INTA:
			SetJumper(m_int.GetCurItemData(m_snd86 & 0x0c), 0x0c);
			break;

		case IDC_SND86ROM:
			SetJumper((IsDlgButtonChecked(IDC_SND86ROM) != BST_UNCHECKED) ? 0x02 : 0x00, 0x02);
			break;

		case IDC_SND86ID:
			SetJumper(m_id.GetCurItemData(m_snd86 & 0xe0), 0xe0);
			break;

		case IDC_SND86DEF:
			Set(0x7f);
			m_dipsw.Invalidate(TRUE);
			break;

		case IDC_SND86DIP:
			OnDipSw();
			break;
	}
	return FALSE;
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT SndOpt86Page::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_DRAWITEM:
			if (LOWORD(wParam) == IDC_SND86DIP)
			{
				UINT8* pBitmap = dipswbmp_getsnd86(m_snd86);
				m_dipsw.Draw((reinterpret_cast<LPDRAWITEMSTRUCT>(lParam))->hDC, pBitmap);
				_MFREE(pBitmap);
			}
			return FALSE;
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * コントロール設定
 * @param[in] cValue 設定値
 */
void SndOpt86Page::Set(UINT8 cValue)
{
	m_snd86 = cValue;
	m_io.SetCurItemData(cValue & 0x01);
	CheckDlgButton(IDC_SND86INT, (cValue & 0x10) ? BST_CHECKED : BST_UNCHECKED);
	m_int.SetCurItemData(cValue & 0x0c);
	m_id.SetCurItemData(cValue & 0xe0);
	CheckDlgButton(IDC_SND86ROM, (cValue & 0x02) ? BST_CHECKED : BST_UNCHECKED);
}

/**
 * 設定
 * @param[in] nAdd 追加ビット
 * @param[in] nRemove 削除ビット
 */
void SndOpt86Page::SetJumper(UINT nAdd, UINT nRemove)
{
	const UINT nValue = (m_snd86 & (~nRemove)) | nAdd;
	if (m_snd86 != static_cast<UINT8>(nValue))
	{
		m_snd86 = static_cast<UINT8>(nValue);
		m_dipsw.Invalidate(TRUE);
	}
}

/**
 * DIPSW をタップした
 */
void SndOpt86Page::OnDipSw()
{
	RECT rect1;
	m_dipsw.GetWindowRect(&rect1);

	RECT rect2;
	m_dipsw.GetClientRect(&rect2);

	POINT p;
	::GetCursorPos(&p);
	p.x += rect2.left - rect1.left;
	p.y += rect2.top - rect1.top;
	p.x /= 8;
	p.y /= 8;
	if ((p.x < 2) || (p.x >= 10) || (p.y < 1) || (p.y >= 3))
	{
		return;
	}
	p.x -= 2;
	m_snd86 ^= (1 << p.x);
	Set(m_snd86);
	m_dipsw.Invalidate(TRUE);
}



// ---- Speak board

static	UINT8	spb = 0;
static	UINT8	spbvrc = 0;

static void setspbVRch(HWND hWnd) {

	SetDlgItemCheck(hWnd, IDC_SPBVRL, spbvrc & 1);
	SetDlgItemCheck(hWnd, IDC_SPBVRR, spbvrc & 2);
}

static void spbcreate(HWND hWnd)
{
	HWND	sub;

	spb = np2cfg.spbopt;

	dlgs_setcbitem(hWnd, IDC_SPBIO, cpIO26, NELEMENTS(cpIO26));
	setsnd26io(hWnd, IDC_SPBIO, spb);
	dlgs_setcbitem(hWnd, IDC_SPBINT, cpInt26, NELEMENTS(cpInt26));
	setsnd26int(hWnd, IDC_SPBINT, spb);
	dlgs_setcbitem(hWnd, IDC_SPBROM, cpAddr, NELEMENTS(cpAddr));
	setsnd26rom(hWnd, IDC_SPBROM, spb);
	spbvrc = np2cfg.spb_vrc;								// ver0.30
	setspbVRch(hWnd);
	SendDlgItemMessage(hWnd, IDC_SPBVRLEVEL, TBM_SETRANGE, TRUE,
															MAKELONG(0, 24));
	SendDlgItemMessage(hWnd, IDC_SPBVRLEVEL, TBM_SETPOS, TRUE,
															np2cfg.spb_vrl);
	SetDlgItemCheck(hWnd, IDC_SPBREVERSE, np2cfg.spb_x);

	sub = GetDlgItem(hWnd, IDC_SPBJMP);
	SetWindowLong(sub, GWL_STYLE, SS_OWNERDRAW +
							(GetWindowLong(sub, GWL_STYLE) & (~SS_TYPEMASK)));
}

static void spbcmdjmp(HWND hWnd) {

	RECT	rect1;
	RECT	rect2;
	POINT	p;
	BOOL	redraw;
	UINT8	b;
	UINT8	bit;

	GetWindowRect(GetDlgItem(hWnd, IDC_SPBJMP), &rect1);
	GetClientRect(GetDlgItem(hWnd, IDC_SPBJMP), &rect2);
	GetCursorPos(&p);
	redraw = FALSE;
	p.x += rect2.left - rect1.left;
	p.y += rect2.top - rect1.top;
	p.x /= 9;
	p.y /= 9;
	if ((p.y < 1) || (p.y >= 3)) {
		return;
	}
	if ((p.x >= 2) && (p.x < 5)) {
		b = spb;
		bit = 0x40 << (2 - p.y);
		switch(p.x) {
			case 2:
				b |= bit;
				break;

			case 3:
				b ^= bit;
				break;

			case 4:
				b &= ~bit;
				break;
		}
		if (spb != b) {
			spb = b;
			setsnd26int(hWnd, IDC_SPBINT, b);
			redraw = TRUE;
		}
	}
	else if (p.x == 7) {
		spb ^= 0x20;
		redraw = TRUE;
	}
	else if ((p.x >= 10) && (p.x < 12)) {
		b = (UINT8)((p.x - 10) << 4);
		if ((spb ^ b) & 0x10) {
			spb &= ~0x10;
			spb |= b;
			setsnd26io(hWnd, IDC_SPBIO, b);
			redraw = TRUE;
		}
	}
	else if ((p.x >= 14) && (p.x < 19)) {
		b = (UINT8)(p.x - 14);
		if ((spb ^ b) & 7) {
			spb &= ~0x07;
			spb |= b;
			setsnd26rom(hWnd, IDC_SPBROM, b);
			redraw = TRUE;
		}
	}
	else if ((p.x >= 21) && (p.x < 24)) {
		spbvrc ^= (UINT8)(3 - p.y);
		setspbVRch(hWnd);
		redraw = TRUE;
	}
	if (redraw) {
		InvalidateRect(GetDlgItem(hWnd, IDC_SPBJMP), NULL, TRUE);
	}
}

static void setspbjmp(HWND hWnd, UINT8 value, UINT8 bit) {

	if ((spb ^ value) & bit) {
		spb &= ~bit;
		spb |= value;
		InvalidateRect(GetDlgItem(hWnd, IDC_SPBJMP), NULL, TRUE);
	}
}

static UINT8 getspbVRch(HWND hWnd) {

	UINT8	ret;

	ret = 0;
	if (GetDlgItemCheck(hWnd, IDC_SPBVRL)) {
		ret += 1;
	}
	if (GetDlgItemCheck(hWnd, IDC_SPBVRR)) {
		ret += 2;
	}
	return(ret);
}

static LRESULT CALLBACK SPBoptDlgProc(HWND hWnd, UINT msg,
													WPARAM wp, LPARAM lp) {
	UINT8	b;
	UINT	update;

	switch(msg) {
		case WM_INITDIALOG:
			spbcreate(hWnd);
			return(TRUE);

		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case IDC_SPBIO:
					setspbjmp(hWnd, getsnd26io(hWnd, IDC_SPBIO), 0x10);
					break;

				case IDC_SPBINT:
					setspbjmp(hWnd, getsnd26int(hWnd, IDC_SPBINT), 0xc0);
					break;

				case IDC_SPBROM:
					setspbjmp(hWnd, getsnd26rom(hWnd, IDC_SPBROM), 0x07);
					break;

				case IDC_SPBDEF:
					spb = 0xd1;
					setsnd26io(hWnd, IDC_SPBIO, spb);
					setsnd26int(hWnd, IDC_SPBINT, spb);
					setsnd26rom(hWnd, IDC_SPBROM, spb);
					spbvrc = 0;
					setspbVRch(hWnd);
					InvalidateRect(GetDlgItem(hWnd, IDC_SPBJMP), NULL, TRUE);
					break;

				case IDC_SPBVRL:
				case IDC_SPBVRR:
					b = getspbVRch(hWnd);
					if ((spbvrc ^ b) & 3) {
						spbvrc = b;
						InvalidateRect(GetDlgItem(hWnd, IDC_SPBJMP),
																NULL, TRUE);
					}
					break;

				case IDC_SPBJMP:
					spbcmdjmp(hWnd);
					break;
			}
			break;

		case WM_NOTIFY:
			if ((((NMHDR *)lp)->code) == (UINT)PSN_APPLY) {
				update = 0;
				if (np2cfg.spbopt != spb) {
					np2cfg.spbopt = spb;
					update |= SYS_UPDATECFG;
				}
				if (np2cfg.spb_vrc != spbvrc) {
					np2cfg.spb_vrc = spbvrc;
					update |= SYS_UPDATECFG;
				}
				b = (UINT8)SendDlgItemMessage(hWnd, IDC_SPBVRLEVEL,
															TBM_GETPOS, 0, 0);
				if (np2cfg.spb_vrl != b) {
					np2cfg.spb_vrl = b;
					update |= SYS_UPDATECFG;
				}
				opngen_setVR(np2cfg.spb_vrc, np2cfg.spb_vrl);
				b = (UINT8)GetDlgItemCheck(hWnd, IDC_SPBREVERSE);
				if (np2cfg.spb_x != b) {
					np2cfg.spb_x = b;
					update |= SYS_UPDATECFG;
				}
				sysmng_update(update);
				return(TRUE);
			}
			break;

		case WM_DRAWITEM:
			if (LOWORD(wp) == IDC_SPBJMP) {
				dlgs_drawbmp(((LPDRAWITEMSTRUCT)lp)->hDC,
											dipswbmp_getsndspb(spb, spbvrc));
			}
			return(FALSE);
	}
	return(FALSE);
}



// ---- JOYPAD

/**
 * @brief PAD ページ
 */
class SndOptPadPage : public CPropPageProc
{
public:
	SndOptPadPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};

//! ボタン
static const UINT s_pad[4][3] =
{
	{IDC_PAD1_1A, IDC_PAD1_2A, IDC_PAD1_RA},
	{IDC_PAD1_1B, IDC_PAD1_2B, IDC_PAD1_RB},
	{IDC_PAD1_1C, IDC_PAD1_2C, IDC_PAD1_RC},
	{IDC_PAD1_1D, IDC_PAD1_2D, IDC_PAD1_RD},
};

/**
 * コンストラクタ
 */
SndOptPadPage::SndOptPadPage()
	: CPropPageProc(IDD_SNDPAD1)
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SndOptPadPage::OnInitDialog()
{
	CheckDlgButton(IDC_JOYPAD1, (np2oscfg.JOYPAD1 & 1) ? BST_CHECKED : BST_UNCHECKED);

	for (UINT i = 0; i < _countof(s_pad); i++)
	{
		for (UINT j = 0; j < 3; j++)
		{
			CheckDlgButton(s_pad[i][j], (np2oscfg.JOY1BTN[i] & (1 << j)) ? BST_CHECKED : BST_UNCHECKED);
		}
	}

	return TRUE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SndOptPadPage::OnOK()
{
	bool bUpdated = false;

	const UINT8 cJoyPad = (np2oscfg.JOYPAD1 & (~1)) | ((IsDlgButtonChecked(IDC_JOYPAD1) != BST_UNCHECKED) ? 1 : 0);
	if (np2oscfg.JOYPAD1 != cJoyPad)
	{
		np2oscfg.JOYPAD1 = cJoyPad;
	}

	for (UINT i = 0; i < _countof(s_pad); i++)
	{
		UINT8 cBtn = 0;
		for (UINT j = 0; j < 3; j++)
		{
			if (IsDlgButtonChecked(s_pad[i][j]) != BST_UNCHECKED)
			{
				cBtn |= (1 << j);
			}
			if (np2oscfg.JOY1BTN[i] != cBtn)
			{
				np2oscfg.JOY1BTN[i] = cBtn;
				bUpdated = true;
			}
		}
	}

	if (bUpdated)
	{
		::joymng_initialize();
		::sysmng_update(SYS_UPDATEOSCFG);
	}
}



// ----

/**
 * サウンド設定
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_sndopt(HWND hwndParent)
{
	std::vector<HPROPSHEETPAGE> hpsp;

	SndOptMixerPage mixer;
	hpsp.push_back(::CreatePropertySheetPage(&mixer.m_psp));

	SndOpt14Page pc980114;
	hpsp.push_back(::CreatePropertySheetPage(&pc980114.m_psp));

	PROPSHEETPAGE psp;
	ZeroMemory(&psp, sizeof(psp));
	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = CWndProc::GetResourceHandle();

	psp.pszTemplate = MAKEINTRESOURCE(IDD_SND26);
	psp.pfnDlgProc = (DLGPROC)Snd26optDlgProc;
	hpsp.push_back(::CreatePropertySheetPage(&psp));

	SndOpt86Page pc980186;
	hpsp.push_back(::CreatePropertySheetPage(&pc980186.m_psp));

	psp.pszTemplate = MAKEINTRESOURCE(IDD_SNDSPB);
	psp.pfnDlgProc = (DLGPROC)SPBoptDlgProc;
	hpsp.push_back(::CreatePropertySheetPage(&psp));

	SndOptPadPage pad;
	hpsp.push_back(::CreatePropertySheetPage(&pad.m_psp));

	std::tstring rTitle(LoadTString(IDS_SOUNDOPTION));

	PROPSHEETHEADER psh;
	ZeroMemory(&psh, sizeof(psh));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	psh.hwndParent = hwndParent;
	psh.hInstance = CWndProc::GetResourceHandle();
	psh.hIcon = LoadIcon(psh.hInstance, MAKEINTRESOURCE(IDI_ICON2));
	psh.nPages = hpsp.size();
	psh.phpage = &hpsp.at(0);
	psh.pszCaption = rTitle.c_str();
	psh.pfnCallback = np2class_propetysheet;
	PropertySheet(&psh);
	InvalidateRect(hwndParent, NULL, TRUE);
}
