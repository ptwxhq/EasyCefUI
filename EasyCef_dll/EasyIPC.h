#pragma once

#include <functional>
#include <unordered_map>
#include <future>

//用于简单处理同步内容

class CEasyFileMap;

class EasyIPCBase
{
public:

	using IPCHandle = HWND;

	using WorkCall = std::function<void(const std::string&, std::string&)>;
	using OnceBlockingWorkCall = std::function<void()>;

	void SetWorkCall(WorkCall call);

	void SetOnceMainThreadBlockingWorkCall(OnceBlockingWorkCall call);

	void ThdRun();

	bool Stop();

	static const std::string GetShareMemName(IPCHandle hFrom, IPCHandle hTo, size_t id);

	bool SendData(IPCHandle handle, const std::string& send, std::string& ret, DWORD timeout);

	virtual ~EasyIPCBase();

	IPCHandle GetHandle() const { return m_hAsServerHandle; }

	virtual bool IsServer() = 0;

	void SetMainThread(DWORD dwId);

	bool IsMainThreadBlocking() const {
		return m_bIsMainThreadBlocking;
	}


	bool TriggerBlockingWorkEvent();


protected:

	static bool SetMemData(const std::string& strSend, const std::string& strMemName, CEasyFileMap* pMem, std::unique_ptr<CEasyFileMap>& pMemLarge, int iFlag);
	static bool GetMemData(std::string& strReturn, int& iFlag, const std::string& strMemName, CEasyFileMap* pMem);

	static LRESULT CALLBACK WORKPROC(HWND, UINT msg, WPARAM wp, LPARAM lp);

	bool Init();
	//阻塞
	void Run();

	DWORD GetMainThreadId() const {
		return m_dwMainThreadId;
	}


	bool m_bIsRunning = false;
	bool m_bIsMainThreadBlocking = false;


	IPCHandle m_hAsServerHandle = nullptr;

	WorkCall m_workCall = nullptr;
	OnceBlockingWorkCall m_OnceBlockingWorkCall = nullptr;

	HANDLE m_hMainBlockingWorkNotify = nullptr;
	HANDLE m_hForceStopWorkNotify = nullptr;

	std::atomic_ulong m_SendRound;

	std::condition_variable m_listDataUpdate;
	std::mutex m_MutWorkthd;
	std::list<std::shared_ptr<std::string>> m_listStrContName;
	
private:
	DWORD m_dwMainThreadId = 0;

};


class EasyIPCServer : public EasyIPCBase
{
	MYDISALLOW_COPY_AND_ASSIGN(EasyIPCServer);
	friend class EasyIPCBase;
public:
	static EasyIPCServer& GetInstance();

	void RemoveClient(int id);

	IPCHandle GetClientHandle(int id);

	size_t GetClientsCount();

	bool IsServer() final {
		return true;
	}

protected:
	EasyIPCServer() = default;

	std::unordered_map<int, IPCHandle> m_clients;
};

class EasyIPCClient : public EasyIPCBase
{
	MYDISALLOW_COPY_AND_ASSIGN(EasyIPCClient);
public:
	static EasyIPCClient& GetInstance();

	void SetServer(IPCHandle handle);

	bool NotifyConnect(int id);

	bool SendDataToServer(const std::string& send, std::string &ret, DWORD timeout);

	bool IsServer() final {
		return false;
	}

	bool IsServerSet();

protected:
	EasyIPCClient() = default;
	IPCHandle m_hServerHandle = nullptr;
};


