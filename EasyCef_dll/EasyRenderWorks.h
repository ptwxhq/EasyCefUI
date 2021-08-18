#pragma once

#include "EasyIPCWorks.h"

class EasyRenderWorks : public EasyIPCWorks
{
	DISALLOW_COPY_AND_ASSIGN(EasyRenderWorks);
	EasyRenderWorks();

	void UIWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData, bool bNeedUIThread) override;

public:
	static EasyRenderWorks& GetInstance();

	bool IsBrowser() override { return false; }

};
