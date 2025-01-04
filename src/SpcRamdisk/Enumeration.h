#pragma once

enum class ETW_VER {
    UNKNOWN = 0,
    SCSI_LOG = 1,           //Win2000 and XP
    ETW_ERROR = 2,          //Win2003 and VISTA
    ETW_EVENT = 3,          //Win7~Win8.0
    NEW_ETW = 4,            //Since Win8.1
};
