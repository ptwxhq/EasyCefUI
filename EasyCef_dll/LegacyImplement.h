#pragma once

#if EASY_LEGACY_API_COMPATIBLE

//这里的代码是直接从原有的项目中复制或者稍微修改的，如果有时间的话就重写一下


#include <list>
#include <unordered_map>


struct DocLoadComplate
{
	int browsr_;
	bool bComp_;
	DocLoadComplate(int id, bool comp) {
		browsr_ = id;
		bComp_ = comp;
	}
};

//指示主框架dom加载完毕（仅用在ui界面）
class DocComplate
{
public:

	static DocComplate& getInst() {
		static DocComplate s_inst;
		return s_inst;
	}

	bool setBrowsr(int id, bool comp);

	bool hitBrowser(int id);

protected:
	DocComplate() {

	}

	virtual ~DocComplate() {

	}

protected:
	std::unordered_map<int, DocLoadComplate> docLoadMap_;
};


struct DectetFrameID
{
	unsigned int browserID_;
	std::string frameID_;
	size_t dectID_;
	int64 identifier_;
	int lastHttpCode_;
};

struct HttpCode
{
	int64 frameID_;
	int lastHttpCode_;
	HttpCode(int64 frameID, int lastHttpCode) {
		frameID_ = frameID;
		lastHttpCode_ = lastHttpCode;
	}
};

class DectetFrameLoad
{
public:
	static DectetFrameLoad& getInst() {
		static DectetFrameLoad s_inst;
		return s_inst;
	}
	virtual ~DectetFrameLoad() {}

	bool Add(unsigned int browser, const std::string& frame, size_t id, int64 identifier) {
		bool ret = false;
		if (hit(browser, frame, id, 200) == false) {
			DectetFrameID dectItem;
			dectItem.browserID_ = browser;
			dectItem.frameID_ = frame;
			dectItem.dectID_ = id;
			dectItem.identifier_ = identifier;
			dectItem.lastHttpCode_ = 200; //默认是200,如果事件没有被触发,页面加载停止发200
			m_dectList.push_back(dectItem);
			ret = true;
		}
		return ret;
	}

	bool Remove(unsigned int browser, const std::string& frame, size_t id) {
		bool ret = false;
		std::list<DectetFrameID>::iterator it = m_dectList.begin();
		for (; it != m_dectList.end(); ++it) {
			if (it->browserID_ == browser && it->frameID_.compare(frame) == 0 && it->dectID_ == id) {
				m_dectList.erase(it);
				ret = true;
				break;
			}
		}
		return ret;
	}

	bool hit(unsigned int browser, const std::string& frame, size_t id, int httpCode) {
		bool ret = false;
		std::list<DectetFrameID>::iterator it = m_dectList.begin();
		for (; it != m_dectList.end(); ++it)
		{
			if (it->browserID_ == browser && it->frameID_.compare(frame) == 0 && it->dectID_ == id) {
				it->lastHttpCode_ = httpCode;
				ret = true;
				break;
			}
		}
		return ret;
	}

protected:
	DectetFrameLoad() {}

private:
	std::list<DectetFrameID> m_dectList;
};


std::string getFramePath(CefRefPtr<CefFrame>& frame);

struct OriginFrameName
{
	int _browserID;
	int64 _frameID;
	std::string _name;
};

class RecordFrameName
{
public:
	static RecordFrameName& getInst() {
		static RecordFrameName s_inst_record_name;
		return s_inst_record_name;
	}
	~RecordFrameName() {}

	bool SaveRecord(const int browser, const int64 frame, const char* name) {
		bool find = false;
		std::list<OriginFrameName>::iterator it = _record_name_list.begin();
		for (; it != _record_name_list.end(); ++it)
		{
			if (it->_browserID == browser && it->_frameID == frame)
			{
				find = true;
				break;
			}
		}
		if (!find)
		{
			OriginFrameName item;
			item._browserID = browser;
			item._frameID = frame;
			item._name = name;
			_record_name_list.push_back(item);
		}
		return !find;
	}

	std::string GetRecord(const int browser, const int64 frame) {
		std::string val;
		std::list<OriginFrameName>::iterator it = _record_name_list.begin();
		for (; it != _record_name_list.end(); ++it)
		{
			if (it->_browserID == browser && it->_frameID == frame)
			{
				val = it->_name;
			}
		}
		return val;
	}

protected:
	RecordFrameName() {}


private:
	std::list<OriginFrameName> _record_name_list;
};

#endif // EASY_LEGACY_API_COMPATIBLE