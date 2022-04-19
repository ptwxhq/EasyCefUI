#pragma once

#include "EasyIPCWorks.h"

class EasyRenderWorks : public EasyIPCWorks
{
	MYDISALLOW_COPY_AND_ASSIGN(EasyRenderWorks);
	EasyRenderWorks();

	void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) override;

public:
	static EasyRenderWorks& GetInstance();

	bool IsBrowser() override { return false; }

};
