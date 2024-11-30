#include "precompile.h"

#define TARGET_MAJOR_VER        6
BOOLEAN IsSupportedOS()
{
    OSVERSIONINFOW info = { 0 };
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    NTSTATUS status = RtlGetVersion(&info);
    if (NT_SUCCESS(status) && info.dwMajorVersion >= TARGET_MAJOR_VER)
        return TRUE;
    return FALSE;
}


EXTERN_C_START
sp_DRIVER_INITIALIZE DriverEntry;
ULONG DriverEntry(
        IN  PVOID DriverObject,
        IN  PVOID RegistryPath
    )
{
    if (IsSupportedOS() == FALSE)
        return STOR_STATUS_UNSUPPORTED_VERSION;

    NTSTATUS status = STATUS_SUCCESS;
    HW_INITIALIZATION_DATA init = { 0 };
    /* Set size of hardware initialization structure. */
    init.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    /* Identify required miniport entry point routines. */
    init.HwInitialize = HwInitialize;
    init.HwStartIo = HwStartIo;                //DEVICE_IO_CTL to SCSI related commands 
    init.HwFindAdapter = HwFindAdapter;        //AddDevice() + IRP_MJ_PNP +  IRP_MN_READ_CONFIG
    init.HwResetBus = HwResetBus;
    init.HwAdapterControl = HwAdapterControl;
    init.HwUnitControl = HwUnitControl;
    init.HwTracingEnabled = HwTracingEnabled;
    init.HwCleanupTracing = HwCleanupTracing;
    init.HwFreeAdapterResources = HwFreeAdapterResources;          //free resources when removing device

    //IRP_MJ_DEVICE_CONTROL + IOCTL code == IOCTL_MINIPORT_PROCESS_SERVICE_IRP
    //to prevent IOCTL request in DPC_LEVEL, define another IOCTL to make sure it is processed in PASSIVE.
    //so define this IOCTL_MINIPORT_PROCESS_SERVICE_IRP
    init.HwProcessServiceRequest = HwProcessServiceRequest;
    //complete NOT FINISHED IRP received in HwProcessServiceRequest. it called when device removed
    init.HwCompleteServiceIrp = HwCompleteServiceIrp;

    /* Specifiy adapter specific information. */
    init.AutoRequestSense = TRUE;
    init.NeedPhysicalAddresses = TRUE;
    init.AdapterInterfaceType = Internal;
    init.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;
    init.TaggedQueuing = TRUE;
    init.MultipleRequestPerLu = TRUE;

    /* Specify support/use SRB Extension for Windows 8 and up */
    init.SrbTypeFlags = SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK;
    init.FeatureSupport = STOR_FEATURE_VIRTUAL_MINIPORT;

    /* Set required extension sizes. */
    init.DeviceExtensionSize = sizeof(SPC_DEVEXT);
    init.SrbExtensionSize = sizeof(SPC_SRBEXT);

    /* Call StorPortInitialize to register with hwInitData */
    status = StorPortInitialize(DriverObject,
        RegistryPath,
        &init,
        NULL);

    return status;
}

EXTERN_C_END
