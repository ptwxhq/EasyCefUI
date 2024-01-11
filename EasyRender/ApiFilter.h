#pragma once


void BlockFlashNotSandboxedCmd();
void HostFilterOn();

void AddLocalHost(const char* domain, const char* ip);
void RemoveLocalHost(const char* domain);