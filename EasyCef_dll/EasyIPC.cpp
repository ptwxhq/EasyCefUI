#include "pch.h"
#include "EasyIPC.h"
#include <thread>
#include <random>



const char* IpcBrowserServerKeyName = "browser_server";

constexpr int g_IpcMaxMemorySize = 1024 * 1024 * 4;

class CEasyFileMap
{
	DISALLOW_COPY_AND_ASSIGN(CEasyFileMap);
public:
	class MAP {
		DISALLOW_COPY_AND_ASSIGN(MAP);
		void* m_pFileMapView = nullptr;
	public:
		operator char* () {
			return (char*)m_pFileMapView;
		}

		MAP() {
		}

		MAP(CEasyFileMap* h, int rw, size_t mapsize) {
			MapSize(h, rw, mapsize);
		}

		~MAP() {
			if (m_pFileMapView)
			{
				UnmapViewOfFile(m_pFileMapView);
				m_pFileMapView = nullptr;
			}
		}

		//由于offset需要1024倍数，废弃
		void* MapSize(CEasyFileMap* h, int rw, size_t mapsize) {
			m_pFileMapView = MapViewOfFile(h->m_hFileMap, rw, 0, 0, mapsize);
			//if (!m_pFileMapView)
			//	throw "map failed";
			return m_pFileMapView;
		}


	};


	CEasyFileMap()
	{
	}

	CEasyFileMap(DWORD size, LPCSTR name, int rw)
	{
		Init(size, name, rw);
	}

	~CEasyFileMap()
	{
		if (m_hFileMap)
		{
			CloseHandle(m_hFileMap);
			m_hFileMap = nullptr;
		}
	}
	
	//size = 0 表示打开
	bool Init(DWORD size, LPCSTR name , int rw)
	{
		if (!m_hFileMap)
		{
			std::string strNew = "_FileMap_";
			strNew += name;
			if (size == 0)
			{
				m_hFileMap = OpenFileMappingA(rw, FALSE, strNew.c_str());//FILE_MAP_READ | FILE_MAP_WRITE
			}
			else
			{
				//PAGE_READONLY		PAGE_READWRITE
				m_hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, rw, 0, size, strNew.c_str());
			}
		
		}

		return !!m_hFileMap;
	}

	operator CEasyFileMap* () {
		return this;
	}


private:
	HANDLE m_hFileMap = nullptr;
};



enum 
{
	UM_IPC_INIT = WM_APP + 1,
	UM_IPC_CLIENT_CONNECT,
	UM_IPC_SEND_WORK_INFO,
};


void EasyIPCBase::SetWorkCall(WorkCall call)
{
	m_workCall = call;
}

void EasyIPCBase::SetOnceMainThreadBlockingWorkCall(OnceBlockingWorkCall call)
{
	m_OnceBlockingWorkCall = call;
}

EasyIPCBase::~EasyIPCBase()
{
	Stop();
}

bool EasyIPCBase::IsSending()
{
	return m_bIsSending;
}

void EasyIPCBase::SetMainThread(DWORD dwId)
{
	m_dwMainThreadId = dwId;
}

bool EasyIPCBase::TriggerBlockingWorkEvent()
{
	if (m_hMainBlockingWorkNotify)
		return !!SetEvent(m_hMainBlockingWorkNotify);
	return false;
}

bool EasyIPCBase::Init()
{
	if (IsWindow(m_hAsServerHandle))
	{
		return true;
	}

	m_hMainBlockingWorkNotify = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hForceStopWorkNotify = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	WCHAR strClassName[] = L"__EasyIpcMsgClass_v1_";

	WNDCLASSW wndcls = { 0 };
	if (!GetClassInfoW(nullptr, strClassName, &wndcls))
	{
		wndcls.lpfnWndProc = WORKPROC;
		wndcls.lpszClassName = strClassName;

		RegisterClassW(&wndcls);
	}

	m_hAsServerHandle = CreateWindowExW(0, strClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);

	return IsWindow(m_hAsServerHandle);
}


void EasyIPCBase::ThdRun()
{
	if (m_bIsRunning)
		return;

	if (IsWindow(m_hAsServerHandle))
	{
		PostMessage(m_hAsServerHandle, WM_QUIT, 0, 0);
	}

	//需要先保证init完成
	std::promise<void> initflag;
	std::thread ([this, &initflag]()
		{
			Init();
			initflag.set_value();
			Run();
		}
	).detach();

	initflag.get_future().wait();
}

bool EasyIPCBase::Stop()
{
	bool bRes = false;

	if (m_hForceStopWorkNotify)
	{
		SetEvent(m_hForceStopWorkNotify);
		CloseHandle(m_hForceStopWorkNotify);
		m_hForceStopWorkNotify = nullptr;
	}

	if (m_hMainBlockingWorkNotify)
	{
		TriggerBlockingWorkEvent();
		CloseHandle(m_hMainBlockingWorkNotify);
		m_hMainBlockingWorkNotify = nullptr;
	}

	if (m_bIsRunning)
	{
		PostMessage(m_hAsServerHandle, WM_QUIT, 0, 0);
		m_bIsRunning = false;

		m_listDataUpdate.notify_all();

		std::lock_guard<std::mutex> lock(m_MutWorkthd);

		bRes = true;
	}


	return bRes;
}


void EasyIPCBase::Run()
{
	if (!m_hAsServerHandle || m_bIsRunning)
		return;


	m_bIsRunning = true;

	SetUserDataPtr(m_hAsServerHandle, this);

	std::vector<std::thread> vecEasyThreadPool;
	const int nEasyThreadPoolCount = 5;
	for (int i = 0; i < nEasyThreadPoolCount; i++)

		vecEasyThreadPool.push_back(std::thread([this]() {

		while (m_bIsRunning)
		{
			std::shared_ptr<std::string> pOnceWork = nullptr;

			{
				std::unique_lock<std::mutex> lock(m_MutWorkthd);

				while (m_listStrContName.empty() && m_bIsRunning)
				{
					m_listDataUpdate.wait(lock);
				}

				if (!m_bIsRunning)
				{
					break;
				}

				pOnceWork = m_listStrContName.front();
				m_listStrContName.pop_front();

				if (!pOnceWork)
				{
					continue;
				}
			}			

			std::string strContName = *pOnceWork;
			{
				auto hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, strContName.c_str());

				if (!hEvent)
				{
					continue;
				}

				try
				{
					if (!m_workCall)
						throw "No work callback";

					size_t srcLen = 0;
					std::string strIn;

					CEasyFileMap shm_obj;

					if (!shm_obj.Init
					(0                    //only open
						, strContName.c_str()              //name
						, FILE_MAP_READ | FILE_MAP_WRITE
					))throw "file map err";

					{
						CEasyFileMap::MAP region(shm_obj, FILE_MAP_READ, sizeof(int) + sizeof(size_t));
						memcpy(&srcLen, region + sizeof(int), sizeof(size_t));
					}

					if (srcLen > 0)
					{
						CEasyFileMap::MAP region(shm_obj, FILE_MAP_READ, sizeof(int) + sizeof(size_t) + srcLen);
						strIn.assign(region + sizeof(int) + sizeof(size_t), srcLen);
					}

					std::string strOut;
					m_workCall(strIn, strOut);

					if (strOut.length() > g_IpcMaxMemorySize - 8)
					{
						LOG(INFO) << GetCurrentProcessId() << "] IPC return too large:(" << strOut.length() << ") max:[" << g_IpcMaxMemorySize;

						throw "return length too large";
					}

					const auto outLen = strOut.length();
					CEasyFileMap::MAP region(shm_obj, FILE_MAP_READ | FILE_MAP_WRITE, outLen + sizeof(size_t) + sizeof(int));
					memset(region, -1, sizeof(int));
					memcpy(region + sizeof(int), &outLen, sizeof(size_t));
					memcpy(region + sizeof(int) + sizeof(size_t), strOut.c_str(), outLen);

				}
				catch (const std::exception& e)
				{
					LOG(WARNING) << GetCurrentProcessId() << "] Err:" << e.what();
				}
				catch (...)
				{
					LOG(WARNING) << GetCurrentProcessId() << "] Err else";
				}

				SetEvent(hEvent);
				CloseHandle(hEvent);
			}
			//LOG(INFO) << GetCurrentProcessId() << "] thread work end" << br;

		}

			}));


	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessageW(&msg, m_hAsServerHandle, 0, 0)) != 0)
	{
		//LOG(INFO) << GetCurrentProcessId() << "] process msg  (" << IsServer() << ") : " << msg.message;

		if (bRet == -1)
		{
			break;
		}
	}

	//LOG(INFO) << GetCurrentProcessId() << "] EasyIPCBase end";
	m_listStrContName.clear();
	m_bIsRunning = false;
	m_hAsServerHandle = nullptr;

	m_listDataUpdate.notify_all();

	for (int i = 0; i < nEasyThreadPoolCount; i++)
	{
		vecEasyThreadPool[i].join();
	}


}

bool EasyIPCBase::SendData(IPCHandle handle, const std::string& send, std::string& ret)
{
	const auto srcLen = send.length();
	if (srcLen > g_IpcMaxMemorySize - 8)
		return false;

	m_bIsSending = true;

	const auto nNowR = ++m_SendRound;

	const bool bIsMainThread = GetCurrentThreadId() == GetMainThreadId();
	if (bIsMainThread)
	{
		m_bIsMainThreadBlocking = true;
	}

	auto strContName = GetShareMemName(m_hAsServerHandle, handle, nNowR);

	//LOG(INFO) << GetCurrentProcessId() << "SendData begin " << strContName << " isMT:" << bIsMainThread << " data:" << send;

	auto hEvent = CreateEventA(nullptr, TRUE, FALSE, strContName.c_str());

	bool bSucc = false;

	try
	{
		if(!hEvent)
			throw "create event failed";


		CEasyFileMap mem_result;
		if (!mem_result.Init(g_IpcMaxMemorySize, strContName.c_str(), PAGE_READWRITE))
		{
			throw "file map failed";
		}


		{
			CEasyFileMap::MAP  region(mem_result, FILE_MAP_READ | FILE_MAP_WRITE, srcLen + sizeof(size_t) + sizeof(int));
			memset(region, 0, sizeof(int));
			memcpy(region + sizeof(size_t), &srcLen, sizeof(size_t));
			memcpy(region + sizeof(size_t) + sizeof(size_t), send.c_str(), srcLen);
		}

		const int sendOK = SendMessage(handle, UM_IPC_SEND_WORK_INFO, nNowR, (LPARAM)m_hAsServerHandle);
		if (sendOK == 1)
		{
			DWORD dwWaitCount = 2;
			HANDLE hEvents[3] = { hEvent, m_hForceStopWorkNotify };
			if (bIsMainThread)
			{
				assert(m_hMainBlockingWorkNotify);
				hEvents[2] = m_hMainBlockingWorkNotify;
				dwWaitCount = 3;
			}

			do
			{
				const DWORD dwWait = WaitForMultipleObjects(dwWaitCount, hEvents, FALSE, 15000);
				if (dwWait == WAIT_OBJECT_0)
				{
					if (bIsMainThread)
					{
						m_bIsMainThreadBlocking = false;
					}

					int iCheck = 0;
					size_t OutLen = 0;

					{
						CEasyFileMap::MAP  region(mem_result, FILE_MAP_READ, sizeof(size_t) + sizeof(int));
						memcpy(&iCheck, region, sizeof(int));
						if (iCheck == -1)
						{
							bSucc = true;
							memcpy(&OutLen, region + sizeof(int), sizeof(size_t));
						}
					}

					if (OutLen > 0)
					{
						CEasyFileMap::MAP  region(mem_result, FILE_MAP_READ, sizeof(size_t) + sizeof(int) + OutLen);
						ret.assign(region + sizeof(size_t) + sizeof(int), OutLen);

					}

					break;
				}
				else if (dwWait == WAIT_OBJECT_0 + 2)
				{
					//执行发过来的同步任务
					if (m_OnceBlockingWorkCall)
					{
						m_OnceBlockingWorkCall();

						m_OnceBlockingWorkCall = nullptr;
					}
				}
				else if (dwWait == WAIT_OBJECT_0 + 1)
				{
					//强制结束
					LOG(WARNING) << GetCurrentProcessId() << "] force end:";
					break;
				}
				else if (dwWait == WAIT_FAILED)
				{
					LOG(WARNING) << GetCurrentProcessId() << "] Failed:" << GetLastError();
					break;
				}
				else if (dwWait == WAIT_TIMEOUT)
				{
					LOG(WARNING) << GetCurrentProcessId() << "] Timeout:" << dwWait;
					break;
				}
				else
				{
					LOG(WARNING) << GetCurrentProcessId() << "] OTHERR:" << dwWait;
					break;
				}
			} while (true);
		}
		else
		{
			LOG(WARNING) << GetCurrentProcessId() << "] SendMessage Err:";
		}

	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << GetCurrentProcessId() << "] Err:" << e.what();
	}
	catch (const char* c)
	{
		LOG(WARNING) << GetCurrentProcessId() << "] Err c:" << c;
	}
	
	if (hEvent)
		CloseHandle(hEvent);

	m_bIsSending = false;

	if (bIsMainThread)
	{
		m_bIsMainThreadBlocking = false;
	}

	//LOG(INFO) << GetCurrentProcessId() << "SendData workend:" << strContName;

	return bSucc;
}

const std::string EasyIPCBase::GetShareMemName(IPCHandle hFrom, IPCHandle hTo)
{
	
#if HAVE_CPP_FORMAT
	return std::format("_EasyIPC_v1_{:X}_{:X}", (size_t)hFrom, (size_t)hTo);
#else
	std::ostringstream ss;
	ss << "_EasyIPC_v1_" << std::hex << hFrom << "_" << hTo;
	return ss.str();
#endif
}

const std::string EasyIPCBase::GetShareMemName(IPCHandle hFrom, IPCHandle hTo, size_t id)
{
#if HAVE_CPP_FORMAT
	return std::format("_EasyIPC_v1_{:X}_{:X}_{:X}", (size_t)hFrom, (size_t)hTo, id);
#else
	std::ostringstream ss;
	ss << "_EasyIPC_v1_" << std::hex << hFrom << "_" << hTo << "_" << id;
	return ss.str();
#endif
}

LRESULT EasyIPCBase::WORKPROC(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case UM_IPC_SEND_WORK_INFO:
		{
			auto pThis = GetUserDataPtr<EasyIPCBase*>(h);

			IPCHandle hSrcHandle = (IPCHandle)lp;

			{
				std::lock_guard<std::mutex> lock(pThis->m_MutWorkthd);

				pThis->m_listStrContName.push_back(std::make_shared<std::string>(GetShareMemName(hSrcHandle, pThis->m_hAsServerHandle, wp)));
			}

			pThis->m_listDataUpdate.notify_one();

			return 1;
		}
		break;
	case UM_IPC_CLIENT_CONNECT:
		{
			auto pThis = GetUserDataPtr<EasyIPCServer*>(h);
			if (pThis)
			{
				pThis->m_clients.insert(std::make_pair(lp, (IPCHandle)wp));
				return 1;
			}
			return 0;
		}
		break;
	default:
		return DefWindowProcW(h, msg, wp, lp);
	}

}


void EasyIPCServer::RemoveClient(int id)
{
	auto client = m_clients.find(id);
	if (client != m_clients.end())
	{
		PostMessage(client->second, WM_CLOSE, 0, 0);
		m_clients.erase(client);
	}
	
}

EasyIPCBase::IPCHandle EasyIPCServer::GetClientHandle(int id)
{
	auto client = m_clients.find(id);
	if (client != m_clients.end())
	{
		return client->second;
	}

	return nullptr;
}

size_t EasyIPCServer::GetClientsCount()
{
	return m_clients.size();
}


EasyIPCServer& EasyIPCServer::GetInstance()
{
	static EasyIPCServer obj;
	return obj;
}

EasyIPCClient& EasyIPCClient::GetInstance()
{
	static EasyIPCClient obj;
	return obj;
}


void EasyIPCClient::SetServer(IPCHandle handle)
{
	m_hServerHandle = handle;
}

bool EasyIPCClient::NotifyConnect(int id)
{
	ASSERT(m_hAsServerHandle);
	if (1 == SendMessage(m_hServerHandle, UM_IPC_CLIENT_CONNECT, (WPARAM)m_hAsServerHandle, id))
	{
		return true;
	}
	return false;
}

bool EasyIPCClient::SendDataToServer(const std::string& send, std::string& ret)
{
	if (m_hServerHandle)
		return SendData(m_hServerHandle, send, ret);
	return false;
}

bool EasyIPCClient::IsServerSet()
{
	return !!m_hServerHandle;
}
