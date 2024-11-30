#include "precompile.h"

SCSI_ADAPTER_CONTROL_STATUS HandleQueryAdapterControlTypes(
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST ctl_list) 
{
//called IRQL == PASSIVE_LEVEL
    CDebugCallInOut inout(__FUNCTION__);
    ULONG max = ctl_list->MaxControlType;

    ctl_list->SupportedTypeList[ScsiQuerySupportedControlTypes] = TRUE;
    if (max >= (ULONG)ScsiStopAdapter)
        ctl_list->SupportedTypeList[ScsiStopAdapter] = TRUE;
    if (max >= (ULONG)ScsiRestartAdapter)
        ctl_list->SupportedTypeList[ScsiRestartAdapter] = TRUE;

    return ScsiAdapterControlSuccess;
}

SCSI_ADAPTER_CONTROL_STATUS HandleScsiStopAdapter(PSPC_DEVEXT devext)
{
//called IRQL == DIRQL.

//ScsiStopAdapter is the Adapter's D0 to D3 state transition event.
//It means "power off on this adapter".
//Normal remove, PCI rebalance, and sleep/hibernation will triggered it.
//VirtualMiniport no need to handle it. 
//But Physical Miniport driver should handle this event.
    CDebugCallInOut inout(__FUNCTION__);
    UNREFERENCED_PARAMETER(devext);
    return ScsiAdapterControlSuccess;
}

SCSI_ADAPTER_CONTROL_STATUS HandleScsiRestartAdapter(PSPC_DEVEXT devext)
{
//called IRQL == DIRQL.

//ScsiRestartAdapter is called when Adapter woke up from D3 to D0 transit.
//sleep/hibernation will triggered it when wake up.
    CDebugCallInOut inout(__FUNCTION__);
    UNREFERENCED_PARAMETER(devext);
    return ScsiAdapterControlSuccess;
}

#if 0
SCSI_ADAPTER_CONTROL_STATUS HandleScsiSetRunningConfig(PSPC_DEVEXT devext)
{
    CDebugCallInOut inout(__FUNCTION__);
    UNREFERENCED_PARAMETER(devext);
    return ScsiAdapterControlSuccess;
}
#endif
