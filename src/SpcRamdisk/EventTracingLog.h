#pragma once

void SetupETW(PDRIVER_OBJECT drvobj, PUNICODE_STRING regpath);
void TeardownETW(PDRIVER_OBJECT drvobj);
void EnableETW(PSPC_DEVEXT devext, BOOLEAN is_enable);

void LogETW_Error(PSPC_SRBEXT srbext);
void LogETW_Info(PSPC_SRBEXT srbext);
void LogETW_Debug(PSPC_SRBEXT srbext);
