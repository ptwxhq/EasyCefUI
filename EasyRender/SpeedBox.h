#ifndef _SPEEDBOX_H
#define _SPEEDBOX_H

// 初始化函数，用于打开或关闭变速功能
BOOL EnableSpeedControl(BOOL bEnable);
// 指定变速比率(0-1.0为减速，1.0-100为加速)
VOID SetSpeed(float fSpeed);

#endif