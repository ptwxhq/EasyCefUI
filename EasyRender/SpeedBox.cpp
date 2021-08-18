#include <windows.h>
#include <detours/detours.h>
#include "SpeedBox.h"

//#pragma comment(lib, "detours.lib")
#pragma comment(lib, "Winmm.lib")

extern "C" {
	static DWORD (WINAPI *Real_GetTickCount)(VOID) = GetTickCount;
	static ULONGLONG(WINAPI* Real_GetTickCount64)(VOID) = GetTickCount64;
	static BOOL (WINAPI *Real_QueryPerformanceCounter)(LARGE_INTEGER *lpPerformanceCount) = QueryPerformanceCounter;
	static DWORD (WINAPI *Real_timeGetTime)(void) = timeGetTime;
};

static CRITICAL_SECTION g_csGetTickCount;
static CRITICAL_SECTION g_csGetTickCount64;
static CRITICAL_SECTION g_csQueryPerformanceCounter;
static CRITICAL_SECTION g_csTimeGetTime;

static float g_fSpeed = 1.0f;

static DWORD WINAPI Mine_GetTickCount(VOID)
{
	DWORD dwTickCount = Real_GetTickCount();

	EnterCriticalSection(&g_csGetTickCount);
	static DWORD dwLastCount = 0;
	static DWORD dwLastResult = 0;
	if (dwLastCount == 0)
	{
		dwLastCount = dwTickCount;
		dwLastResult = dwTickCount;
	}
	else
	{
		dwLastResult += (DWORD)((dwTickCount - dwLastCount) * g_fSpeed + 0.5);
		dwLastCount = dwTickCount;
		dwTickCount = dwLastResult;
	}
	LeaveCriticalSection(&g_csGetTickCount);
	return dwTickCount;
}

static ULONGLONG WINAPI Mine_GetTickCount64(VOID)
{
	ULONGLONG dwTickCount = Real_GetTickCount();

	EnterCriticalSection(&g_csGetTickCount64);
	static ULONGLONG ullLastCount = 0;
	static ULONGLONG ullLastResult = 0;
	if (ullLastCount == 0)
	{
		ullLastCount = dwTickCount;
		ullLastResult = dwTickCount;
	}
	else
	{
		ullLastResult += (ULONGLONG)((dwTickCount - ullLastCount) * g_fSpeed + 0.5);
		ullLastCount = dwTickCount;
		dwTickCount = ullLastResult;
	}
	LeaveCriticalSection(&g_csGetTickCount64);
	return dwTickCount;
}

static BOOL WINAPI Mine_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	BOOL bResult = Real_QueryPerformanceCounter(lpPerformanceCount);

	EnterCriticalSection(&g_csQueryPerformanceCounter);
	static LARGE_INTEGER liLastCount = {0};
	static LARGE_INTEGER liLastResult = {0};
	if (liLastCount.QuadPart == 0)
	{
		liLastCount.QuadPart = lpPerformanceCount->QuadPart;
		liLastResult.QuadPart = lpPerformanceCount->QuadPart;
	}
	else
	{
		liLastResult.QuadPart += (LONGLONG)((lpPerformanceCount->QuadPart - liLastCount.QuadPart) * g_fSpeed + 0.5);
		liLastCount.QuadPart = lpPerformanceCount->QuadPart;
		lpPerformanceCount->QuadPart = liLastResult.QuadPart;
	}
	LeaveCriticalSection(&g_csQueryPerformanceCounter);
	return bResult;
}

static DWORD WINAPI Mine_timeGetTime(void)
{
	DWORD dwTickCount = Real_timeGetTime();

	EnterCriticalSection(&g_csTimeGetTime);
	static DWORD dwLastCount = 0;
	static DWORD dwLastResult = 0;
	if (dwLastCount == 0)
	{
		dwLastCount = dwTickCount;
		dwLastResult = dwTickCount;
	}
	else
	{
		dwLastResult += (DWORD)((dwTickCount - dwLastCount) * g_fSpeed + 0.5);
		dwLastCount = dwTickCount;
		dwTickCount = dwLastResult;
	}
	LeaveCriticalSection(&g_csTimeGetTime);
	return dwTickCount;
}

BOOL EnableSpeedControl(BOOL bEnable)
{
	static BOOL bEnabled = FALSE;
	if ((bEnable && bEnabled) || (!bEnable && !bEnabled))
		return FALSE;

	BOOL bSuccessed;
	if (bEnable)
	{
		InitializeCriticalSection(&g_csGetTickCount);
		InitializeCriticalSection(&g_csGetTickCount64);
		InitializeCriticalSection(&g_csQueryPerformanceCounter);
		InitializeCriticalSection(&g_csTimeGetTime);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		*(PVOID *)&Real_GetTickCount = DetourCodeFromPointer((PVOID)GetTickCount, NULL);
 		DetourAttach(&(PVOID&)Real_GetTickCount, Mine_GetTickCount);
		DetourAttach(&(PVOID&)Real_QueryPerformanceCounter, Mine_QueryPerformanceCounter);
		DetourAttach(&(PVOID&)Real_timeGetTime, Mine_timeGetTime);

		bSuccessed = DetourTransactionCommit() == 0;
		bEnabled = TRUE;
	}
	else
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		DetourDetach(&(PVOID&)Real_GetTickCount, Mine_GetTickCount);
		DetourDetach(&(PVOID&)Real_GetTickCount64, Mine_GetTickCount64);
		DetourDetach(&(PVOID&)Real_QueryPerformanceCounter, Mine_QueryPerformanceCounter);
		DetourDetach(&(PVOID&)Real_timeGetTime, Mine_timeGetTime);

		bSuccessed = DetourTransactionCommit() == 0;
		DeleteCriticalSection(&g_csGetTickCount);
		DeleteCriticalSection(&g_csGetTickCount64);
		DeleteCriticalSection(&g_csQueryPerformanceCounter);
		DeleteCriticalSection(&g_csTimeGetTime);
		bEnabled = FALSE;
	}
	return bSuccessed;
}

VOID SetSpeed(float fSpeed)
{
	g_fSpeed = fSpeed;
}
