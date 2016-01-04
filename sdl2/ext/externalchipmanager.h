/**
 * @file	externalchipmanager.h
 * @brief	外部チップ管理クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "gimic/gimic.h"
#include "spfm/spfmmanager.h"

/**
 * @brief 外部チップ管理クラス
 */
class CExternalChipManager
{
public:
	static CExternalChipManager* GetInstance();

	CExternalChipManager();
	~CExternalChipManager();
	void Initialize();
	void Deinitialize();
	IExternalChip* GetInterface(IExternalChip::ChipType nChipType, UINT nClock);
	void Release(IExternalChip* pChip);
	void Reset();
	void Mute(bool bMute);

private:
	static CExternalChipManager sm_instance;	//!< 唯一のインスタンスです

	CGimic m_gimic;
	CSpfmManager m_spfm;
	IExternalChip* GetInterfaceInner(IExternalChip::ChipType nChipType, UINT nClock);
};

/**
 * インスタンスを得る
 * @return インスタンス
 */
inline CExternalChipManager* CExternalChipManager::GetInstance()
{
	return &sm_instance;
}
