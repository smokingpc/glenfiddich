#include "precompile.h"
//note: Telemetry
//Since Win2016: StorPortLogTelemetry()
//Since Win10 1903 : StorPortLogTelemetryEx()

//note: SystemEventLog
//Since Win7 ~ Win8.0: StorPortLogSystemEvent()
//Win2003 ~ Win7: StorPortLogError()
//WinXP and BEFORE: ScsiportLogError()

//note: ETW
//Since Win8.1: StorPortEtwEvent2() ~ StorPortEtwEvent8()
//Since Win8.1: StorPortEtwChannelEvent2() ~ StorPortEtwChannelEvent8()
//Since Win11 24H2 and Win2025: StorPortNvmeMiniportEvent()

static ETW_VER EtwVer = ETW_VER::UNKNOWN;

void SetupETW(PDRIVER_OBJECT drvobj, PUNICODE_STRING regpath)
{
    UNREFERENCED_PARAMETER(drvobj);
    UNREFERENCED_PARAMETER(regpath);

    RTL_OSVERSIONINFOEXW verinfo = {0};
    verinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    NTSTATUS status = RtlGetVersion((PRTL_OSVERSIONINFOW) &verinfo);

    //I need to know the current version to determine which API I should use...
    if(!NT_SUCCESS(status) || verinfo.dwMajorVersion <= 5)
    {
        EtwVer = ETW_VER::SCSI_LOG;
        return;
    }

    //Window 2000 == NT 5.5
    //WindowXP == NT 5.1
    //Window Server 2003 / Window Home Server / WinXP x64 == NT 5.2
    //Window Server 2008 and VISTA == NT 6.0
    //Window Server 2008R2 and Win7 == NT 6.1
    //Win2012 and Win8 == NT 6.2
    //Win2012R2 and Win8.1 == NT 6.3
    //refer to https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_osversioninfoexw

    if (verinfo.dwMajorVersion > 6)
    {
        //dwMajorVersion == 10
        EtwVer = ETW_VER::NEW_ETW;
    }
    else if(6==verinfo.dwMajorVersion)
    {
        switch(verinfo.dwMinorVersion)
        {
        case 0:
            EtwVer = ETW_VER::ETW_ERROR;
            break;
        case 1:
        case 2:
            EtwVer = ETW_VER::ETW_EVENT;
            break;
        case 3:
            EtwVer = ETW_VER::NEW_ETW;
            break;
        }
    }

    //todo: write first log
}
void TeardownETW(PDRIVER_OBJECT drvobj)
{
    UNREFERENCED_PARAMETER(drvobj);
    //todo: write last log
}
void EnableETW(PSPC_DEVEXT devext, BOOLEAN is_enable)
{
    devext->EtwEnabled = is_enable;
    //todo: write log to indicate log on/off.
}

void LogETW_Error(PSPC_SRBEXT srbext)
{
    UNREFERENCED_PARAMETER(srbext);
}
void LogETW_Info(PSPC_SRBEXT srbext)
{
    UNREFERENCED_PARAMETER(srbext);
}
void LogETW_Debug(PSPC_SRBEXT srbext)
{
    UNREFERENCED_PARAMETER(srbext);
}
