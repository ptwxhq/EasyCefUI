#pragma once

#include <functional>
#include <unordered_map>
#include <future>

extern const char* IpcBrowserServerKeyName /*= "browser_server"*/;

//用于简单处理同步内容


class EasyIPCBase
{
public:

	using IPCHandle = HWND;

	typedef std::function<void(const std::string&, std::string&)> WorkCall;
	typedef std::function<void()> OnceBlockingWorkCall;

	virtual void SetWorkCall(WorkCall call);

	virtual void SetOnceMainThreadBlockingWorkCall(OnceBlockingWorkCall call);

	virtual bool Init();

	//阻塞
	virtual void Run();

	virtual bool Stop();

	//使用这个则不能自己init
	virtual void ThdRun();

	static LRESULT CALLBACK WORKPROC(HWND, UINT msg, WPARAM wp, LPARAM lp);

	static const std::string GetShareMemName(IPCHandle hFrom, IPCHandle hTo);
	static const std::string GetShareMemName(IPCHandle hFrom, IPCHandle hTo, size_t id);

	virtual bool SendData(IPCHandle handle, const std::string& send, std::string& ret);

	virtual ~EasyIPCBase();

	IPCHandle GetHandle() const { return m_hAsServerHandle; }

	virtual bool IsServer() = 0;

	bool IsSending();

	void SetMainThread(DWORD dwId);

	DWORD GetMainThreadId() const {
		return m_dwMainThreadId;
	}

	bool IsMainThreadBlocking() const {
		return m_bIsMainThreadBlocking;
	}

	bool TriggerBlockingWorkEvent();


protected:
	bool m_bIsRunning = false;
	bool m_bIsSending = false;
	bool m_bIsMainThreadBlocking = false;


	IPCHandle m_hAsServerHandle = nullptr;

	WorkCall m_workCall = nullptr;
	OnceBlockingWorkCall m_OnceBlockingWorkCall = nullptr;

	HANDLE m_hMainBlockingWorkNotify = nullptr;
	HANDLE m_hForceStopWorkNotify = nullptr;

	std::atomic_ulong m_SendRound;

	//to handle, flag
	std::unordered_map<IPCHandle, std::unique_ptr<std::promise<void>>> m_MsgShareMemQueue;

	std::condition_variable m_listDataUpdate;
	std::mutex m_MutWorkthd;
	std::list<std::shared_ptr<std::string>> m_listStrContName;
	
private:
	DWORD m_dwMainThreadId = 0;

};


class EasyIPCServer : public EasyIPCBase
{
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
public:
	static EasyIPCClient& GetInstance();

	void SetServer(IPCHandle handle);

	bool NotifyConnect(int id);

	bool SendDataToServer(const std::string& send, std::string &ret);

	bool IsServer() final {
		return false;
	}

	bool IsServerSet();

protected:
	EasyIPCClient() = default;
	IPCHandle m_hServerHandle = nullptr;
};


