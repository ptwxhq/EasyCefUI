#include "pch.h"
#include "LegacyImplement.h"

#if EASY_LEGACY_API_COMPATIBLE

std::string getFramePath(CefRefPtr<CefFrame>& frame)
{
	std::string path;
	if (frame)
	{
		CefRefPtr<CefFrame> vist = frame;
		while (vist && !vist->IsMain())
		{
			path += std::to_string(vist->GetIdentifier());
			path.append(",");
			vist = vist->GetParent();
		}
		if (vist && vist->IsMain())
		{
			path += std::to_string(vist->GetIdentifier());
		}

	}
	return path;
}

bool DocComplate::setBrowsr(int id, bool comp)
{
	auto it = docLoadMap_.find(id);
	if (it != docLoadMap_.end())
	{
		it->second.bComp_ = comp;
	}
	else {
		DocLoadComplate item(id, comp);
		docLoadMap_.insert(std::make_pair(id, item));
	}
	return true;
}

bool DocComplate::hitBrowser(int id)
{
	bool bFind = false;
	auto it = docLoadMap_.find(id);
	if (it != docLoadMap_.end())
	{
		bFind = it->second.bComp_;
	}
	return bFind;
}






#endif // EASY_LEGACY_API_COMPATIBLE