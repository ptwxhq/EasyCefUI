// EasyRender.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyRender.h"

#include <cmath>

#include "SpeedBox.h"

#include "../EasyCef_dll/Export.h"
#ifdef _DEBUG
#pragma comment(lib, "Debug/easycef_d.lib")
#else
#pragma comment(lib, "Release/easycef.lib")
#endif // DEBUG


void SpeedWork(float val)
{
    static bool bSet = false;
    if (fabs(val - 1.0) < 0.01)
    {
        if (bSet)
        {
            //设置启动之后就不要去关了，原先是这样的，应该原先有什么问题吧，就按这个来吧
            EnableSpeedControl(TRUE);
            SetSpeed(1.f);
        }
        else
        {
            EnableSpeedControl(FALSE);
        }
       
    }
    else
    {
        bSet = true;
        EnableSpeedControl(TRUE);
        SetSpeed(val);
    }
}







int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    if (__argc < 2)
    {
        return 0;
    }

    using namespace EASYCEF;

    SetSpeedUpWork(SpeedWork);

    EASYCEF::EasyInitConfig config;
    config.ProcessType = 2;

    return InitEasyCef(hInstance, nullptr, &config);


   // return testwinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
