/**
 * @file	rebirth.h
 * @brief	SCCI アクセス クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "..\externalchip.h"

class SoundInterfaceManager;
class SoundChip;

/**
 * @brief SCCI アクセス クラス
 */
class CRebirth : public IExternalChip
{
public:
	CRebirth();
	virtual ~CRebirth();
	virtual bool Initialize();
	virtual void Deinitialize();
	virtual bool IsEnabled();
	virtual bool IsBusy();
	virtual void Reset();
	virtual void WriteRegister(UINT nAddr, UINT8 cData);

private:
	HMODULE m_hModule;					//!< モジュール	
	SoundInterfaceManager* m_pManager;	//!< マネージャ
	SoundChip* m_pChip;					//!< サウンド チップ インターフェイス
	static bool sm_bOnce;				//!< once
};

/**
 * RE:birth は有効?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CRebirth::IsEnabled()
{
	return (m_pChip != NULL);
}
