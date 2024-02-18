#include "pch.h"
#include "EasyIPC.h"
#include <thread>
#include <random>



constexpr int g_IpcNormalMaxMemorySize = 1024 * 4;	 //4K大小通常已经够用了，目前更大数据的会额外自动处理，且最大可达到size_t
constexpr int g_IpcNormalMaxDataSize = g_IpcNormalMaxMemorySize - sizeof(int) - sizeof(size_t);

enum MEMFLAG
{
	E_UNSET        = 0x00000000,
	E_SEND         = 0x0000f000,
	E_SEND_LARGE   = 0x0000f001,
	E_RETURN       = 0xffff0000,
	E_RETURN_LARGE = 0xffff00f0,
};

class CEasyFileMap
{
	MYDISALLOW_COPY_AND_ASSIGN(CEasyFileMap);
public:
	class MAP {
		MYDISALLOW_COPY_AND_ASSIGN(MAP);
		void* m_pFileMapView = nullptr;
	public:
		operator char* () {
			return (char*)m_pFileMapView;
		}

		MAP() {
		}

		explicit MAP(CEasyFileMap* h, bool write, size_t mapsize) {
			MapSize(h, write, mapsize);
		}

		~MAP() {
			Unmap();
		}

		//由于offset需要1024倍数，废弃
		void* MapSize(CEasyFileMap* h, bool write, size_t mapsize) {
			Unmap();

			DWORD access = FILE_MAP_READ;

			if (write)
			{
				access |= FILE_MAP_WRITE;
			}

			m_pFileMapView = MapViewOfFile(h->m_hFileMap, access, 0, 0, mapsize);
			//if (!m_pFileMapView)
			//	throw "map failed";
			return m_pFileMapView;
		}

		void Unmap() {
			if (m_pFileMapView)
			{
				UnmapViewOfFile(m_pFileMapView);
				m_pFileMapView = nullptr;
			}
		}


	};


	CEasyFileMap()
	{
	}

	explicit CEasyFileMap(DWORD size, LPCSTR name, bool write)
	{
		Init(size, name, write);
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
	bool Init(size_t size, LPCSTR name , bool write)
	{
		if (!m_hFileMap)
		{
			std::string strNew = "_FileMap_";
			strNew += name;
			if (size == 0)
			{
				DWORD dwRw = FILE_MAP_READ;
				if (write)
				{
					dwRw |= FILE_MAP_WRITE;
				}


				m_hFileMap = OpenFileMappingA(dwRw, FALSE, strNew.c_str());
			}
			else
			{
				DWORD dwLow = size;
				DWORD dwHigh = 0;
#ifdef _WIN64
				dwHigh = size >> 32;
#endif // _WIN64
				
				m_hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, dwHigh, dwLow, strNew.c_str());
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

void EasyIPCBase::SetOnceMainThreadBlockingWorkCall(FuncVoidWorkCall call)
{
	m_OnceBlockingWorkCall = call;
}

void EasyIPCBase::SetStopWorkingWorkCall(FuncVoidWorkCall call)
{
	m_StopWorkingCall = call;
}

EasyIPCBase::~EasyIPCBase()
{
	Stop();
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

		if (IsServer())
		{
			if (m_hRunningWork)
			{
				WaitForSingleObject(m_hRunningWork, INFINITE);
			}
		}

		bRes = true;
	}


	return bRes;
}


void EasyIPCBase::Run()
{
	if (!m_hAsServerHandle || m_bIsRunning)
		return;


	m_bIsRunning = true;
	if (IsServer())
	{
		m_hRunningWork = CreateEventA(nullptr, FALSE, FALSE, nullptr);
		if (!m_hRunningWork)
			return;
	}


	SetUserDataPtr(m_hAsServerHandle, this);

	std::vector<std::thread> vecEasyThreadPool;
	const int nEasyThreadPoolCount = IsServer() ? 6 : 4;
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
				HANDLE hLargeEvent = nullptr;
				std::unique_ptr<CEasyFileMap> pMem_large;

				if (!hEvent)
				{
					continue;
				}

				try
				{
					if (!m_workCall)
						throw "No work callback";

					std::string strIn;

					CEasyFileMap shm_obj;

					if (!shm_obj.Init(0, strContName.c_str(), true))
						throw "file map err";

					int iCheck = E_UNSET;

					if (!GetMemData(strIn, iCheck, strContName, &shm_obj))
					{
						throw "get data err";
					}

					if (iCheck == E_SEND || iCheck == E_SEND_LARGE)
					{
					}
					else
					{
						throw "invalid mmdata";
					}
					

					std::string strOut;
					m_workCall(strIn, strOut);

					const MEMFLAG setflag = strOut.length() > g_IpcNormalMaxDataSize ? E_RETURN_LARGE : E_RETURN;

					const auto strLarReturnContName = "RT" + strContName;

					SetMemData(strOut, strLarReturnContName, &shm_obj, pMem_large, setflag);

					if (pMem_large)
					{
						hLargeEvent = CreateEventA(nullptr, TRUE, FALSE, strLarReturnContName.c_str());

						assert(hLargeEvent);
					}


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

				if (hLargeEvent)
				{
					if (WAIT_TIMEOUT == WaitForSingleObject(hLargeEvent, 30000))
					{
						LOG(WARNING) << GetCurrentProcessId() << "] Large return timeout";
					}
					CloseHandle(hLargeEvent);
				}

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

	if (m_StopWorkingCall)
	{
		m_StopWorkingCall();
	}
	m_listDataUpdate.notify_all();

	for (int i = 0; i < nEasyThreadPoolCount; i++)
	{
		vecEasyThreadPool[i].join();
	}

	if (IsServer())
	{
		SetEvent(m_hRunningWork);
		CloseHandle(m_hRunningWork);
		m_hRunningWork = nullptr;
	}


}

bool EasyIPCBase::SendData(IPCHandle handle, const std::string& send, std::string& ret, DWORD timeout)
{
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
		if (!mem_result.Init(g_IpcNormalMaxMemorySize, strContName.c_str(), true))
		{
			throw "file map failed";
		}

		std::unique_ptr<CEasyFileMap> pMem_large;

		const auto srcLen = send.length();

		MEMFLAG flag = srcLen > g_IpcNormalMaxDataSize ? E_SEND_LARGE : E_SEND;

		if (!SetMemData(send, strContName, &mem_result, pMem_large, flag))
		{
			throw "file map set failed";
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
				const DWORD dwWait = WaitForMultipleObjects(dwWaitCount, hEvents, FALSE, timeout == 0 ? INFINITE : timeout);
				if (dwWait == WAIT_OBJECT_0)
				{
					if (bIsMainThread)
					{
						m_bIsMainThreadBlocking = false;
					}

					int iCheck = E_UNSET;
					if (GetMemData(ret, iCheck, strContName, &mem_result))
					{
						if (iCheck == E_RETURN || iCheck == E_RETURN_LARGE)
						{
							bSucc = true;
						}
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
					if (IsServer())
					{
						assert(0 && "M->S IPC Timeout");
					}
					else
					{
						assert(0 && "S->M IPC Timeout");
					}
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

	if (bIsMainThread)
	{
		m_bIsMainThreadBlocking = false;
	}

	//LOG(INFO) << GetCurrentProcessId() << "SendData workend:" << strContName;

	return bSucc;
}

const std::string EasyIPCBase::GetShareMemName(IPCHandle hFrom, IPCHandle hTo, size_t id)
{
	return std::format("_EasyIPC_v1_{:X}_{:X}_{:X}", (size_t)hFrom, (size_t)hTo, id);
}

EasyIPCBase::EasyIPCBase(bool bServer) : m_bIsServer(bServer)
{
}

bool EasyIPCBase::SetMemData(const std::string& strSend, const std::string& strMemName, CEasyFileMap* pMem, std::unique_ptr<CEasyFileMap>& pMemLarge, int iFlag)
{
	const auto srcLen = strSend.length();
	const bool bIsLargeData = srcLen > g_IpcNormalMaxDataSize;
	bool bSucc = false;
	std::string strLargeMemName;

	try
	{

		size_t nNeedSize = 0;
		if (bIsLargeData)
		{
			pMemLarge = std::make_unique<CEasyFileMap>();

			if (iFlag != E_RETURN_LARGE)
			{
				strLargeMemName = "LAR" + strMemName;
			}
			else
			{
				strLargeMemName = strMemName;
			}

			if (!pMemLarge->Init(srcLen, strLargeMemName.c_str(), true))
			{
				throw "lar file map failed";
			}
		}
		else
		{
			nNeedSize += srcLen;
		}

		MEMFLAG flag = (MEMFLAG)iFlag;

		
		CEasyFileMap::MAP region(pMem, true, nNeedSize);

		size_t nCurWritePos = 0;
		memcpy(region, &flag, sizeof(MEMFLAG));
		nCurWritePos += sizeof(MEMFLAG);
		memcpy(region + nCurWritePos, &srcLen, sizeof(srcLen));
		nCurWritePos += sizeof(srcLen);

		if (bIsLargeData)
		{
			const auto nLMN = strLargeMemName.size();
			memcpy(region + nCurWritePos, &nLMN, sizeof(nLMN));
			nCurWritePos += sizeof(MEMFLAG);
			memcpy(region + nCurWritePos, strLargeMemName.c_str(), nLMN);

			CEasyFileMap::MAP regionLar(pMemLarge.get(), true, srcLen);
			memcpy(regionLar, strSend.data(), srcLen);
		}
		else
		{
			memcpy(region + nCurWritePos, strSend.data(), srcLen);
		}

		bSucc = true;
	}
	catch (const char* c)
	{
		LOG(INFO) << GetCurrentProcessId() << "] Err c:" << c;
	}



	return bSucc;
}

bool EasyIPCBase::GetMemData(std::string& strReturn, int& iFlag, const std::string& strMemName, CEasyFileMap* pMem)
{
	bool bSucc = false;
	MEMFLAG iCheck = E_UNSET;
	size_t OutLen = 0;

	CEasyFileMap::MAP region(pMem, false, 0);

	memcpy(&iCheck, region, sizeof(MEMFLAG));
	size_t nCurReadPos = sizeof(MEMFLAG);
	memcpy(&OutLen, region + nCurReadPos, sizeof(OutLen));
	nCurReadPos += sizeof(OutLen);

	iFlag = iCheck;

	switch (iCheck)
	{
	case E_UNSET:
		break;
	case E_SEND:
	case E_RETURN:

		bSucc = true;
		if (OutLen > 0)
		{
			strReturn.assign(region + nCurReadPos, OutLen);
		}

		break;
	case E_SEND_LARGE:
	case E_RETURN_LARGE:
	{
		bSucc = true;
		size_t nLMN = 0;

		memcpy(&nLMN, region + nCurReadPos, sizeof(nLMN));
		nCurReadPos += sizeof(nLMN);

		std::string strContName;
		strContName.assign(region + nCurReadPos, nLMN);


		CEasyFileMap memLarge(0, strContName.c_str(), false);
		CEasyFileMap::MAP regionLar(memLarge, false , OutLen);
		
		strReturn.assign(regionLar, OutLen);

		if (E_RETURN_LARGE == iCheck)
		{
			auto hEvt = OpenEventA(EVENT_MODIFY_STATE, FALSE, strContName.c_str());
			if (hEvt) {
				SetEvent(hEvt);
				CloseHandle(hEvt);
			}
		}
	}
	break;
	default:
		break;
	}

	return bSucc;
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
		PostMessage(client->second, WM_QUIT, 0, 0);
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

EasyIPCServer::EasyIPCServer() : EasyIPCBase(true)
{
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

bool EasyIPCClient::SendDataToServer(const std::string& send, std::string& ret, DWORD timeout)
{
	if (m_hServerHandle)
		return SendData(m_hServerHandle, send, ret, timeout);
	return false;
}

bool EasyIPCClient::IsServerSet()
{
	return !!m_hServerHandle;
}

EasyIPCClient::EasyIPCClient() : EasyIPCBase(false)
{
}
