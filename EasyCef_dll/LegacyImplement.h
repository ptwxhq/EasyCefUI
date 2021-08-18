#pragma once

#if EASY_LEGACY_API_COMPATIBLE

//����Ĵ�����ֱ�Ӵ�ԭ�е���Ŀ�и��ƻ�����΢�޸ĵģ������ʱ��Ļ�����дһ��


#include "LegacyExports.h"


#include "LegacyCodes/WebkitEcho.h"

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

//ָʾ�����dom������ϣ�������ui���棩
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
	unsigned int dectID_;
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

	bool Add(unsigned int browser, const std::string& frame, unsigned int id, int64 identifier) {
		bool ret = false;
		if (hit(browser, frame, id, 200) == false) {
			DectetFrameID dectItem;
			dectItem.browserID_ = browser;
			dectItem.frameID_ = frame;
			dectItem.dectID_ = id;
			dectItem.identifier_ = identifier;
			dectItem.lastHttpCode_ = 200; //Ĭ����200,����¼�û�б�����,ҳ�����ֹͣ��200
			m_dectList.push_back(dectItem);
			ret = true;
		}
		return ret;
	}

	bool Remove(unsigned int browser, const std::string& frame, unsigned int id) {
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

	bool hit(unsigned int browser, const std::string& frame, unsigned int id, int httpCode) {
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

	void getNotifyLoadStop(unsigned int browser, std::vector<HttpCode>& frameIDs) {
		std::list<DectetFrameID>::iterator it = m_dectList.begin();
		for (; it != m_dectList.end(); ++it)
		{
			if (it->browserID_ == browser)
			{
				HttpCode item(it->identifier_, it->lastHttpCode_);
				frameIDs.push_back(item);
			}
		}
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