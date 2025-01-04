#include "precompile.h"
void CopyToCdbBuffer(PUCHAR& buffer, ULONG& buf_size, PVOID page, ULONG& copy_size, ULONG& ret_size)
{
    if (copy_size > buf_size)
        copy_size = buf_size;

    StorPortCopyMemory(buffer, page, copy_size);
    buf_size -= copy_size;  //calculate "how many bytes left for my pending copy"?
    ret_size += copy_size;
    buffer += copy_size;
}
void FillParamHeader(PMODE_PARAMETER_HEADER header)
{
    header->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) - sizeof(header->ModeDataLength);
    header->MediumType = DIRECT_ACCESS_DEVICE;
    header->DeviceSpecificParameter = 0;
    header->BlockDescriptorLength = 0;  //I don't want reply BlockDescriptor  :p
}
void FillParamHeader10(PMODE_PARAMETER_HEADER10 header)
{
    //MODE_PARAMETER_HEADER10 header = { 0 };
    USHORT data_size = sizeof(MODE_PARAMETER_HEADER10);
    USHORT mode_data_size = data_size - sizeof(header->ModeDataLength);
    REVERSE_BYTES_2(header->ModeDataLength, &mode_data_size);
    header->MediumType = DIRECT_ACCESS_DEVICE;
    header->DeviceSpecificParameter = 0;
    //I don't want reply BlockDescriptor, so dont set BlockDescriptorLength field  :p
}
void FillModePage_Caching(PMODE_CACHING_PAGE page)
{
    page->PageCode = MODE_PAGE_CACHING;
    page->PageLength = (UCHAR)(sizeof(MODE_CACHING_PAGE) - 2); //sizeof(MODE_CACHING_PAGE) - sizeof(page->PageCode) - sizeof(page->PageLength)
}
void FillModePage_InfoException(PMODE_INFO_EXCEPTIONS page)
{
    page->PageCode = MODE_PAGE_FAULT_REPORTING;
    page->PageLength = (UCHAR)(sizeof(MODE_INFO_EXCEPTIONS) - 2); //sizeof(MODE_INFO_EXCEPTIONS) - sizeof(page->PageCode) - sizeof(page->PageLength)
    page->ReportMethod = 5;  //Generate no sense
}
void FillModePage_Control(PMODE_CONTROL_PAGE page)
{
    //all fields of MODE_CONTROL_PAGE refer to Seagate SCSI reference "Control Mode page (table 302)" 
    page->PageCode = MODE_PAGE_CONTROL;
    page->PageLength = (UCHAR)(sizeof(MODE_CONTROL_PAGE) - 2); //sizeof(MODE_CONTROL_PAGE) - sizeof(page->PageCode) - sizeof(page->PageLength)
    page->QueueAlgorithmModifier = 0;
}
void ReplyModePageCaching(
    PSPC_DEVEXT devext,
    PUCHAR& buffer, 
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size)
{
    MODE_CACHING_PAGE page = { 0 };
    mode_page_size = sizeof(MODE_CACHING_PAGE);
    FillModePage_Caching(&page);
    page.ReadDisableCache = !devext->ReadCacheEnabled;
    page.WriteCacheEnable = devext->WriteCacheEnabled;
    CopyToCdbBuffer(buffer, buf_size, &page, mode_page_size, ret_size);
}
void ReplyModePageControl(
    PSPC_DEVEXT devext,
    PUCHAR& buffer, 
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size)
{
    MODE_CONTROL_PAGE page = { 0 };
    UNREFERENCED_PARAMETER(devext);
    mode_page_size = sizeof(MODE_CONTROL_PAGE);
    FillModePage_Control(&page);
    CopyToCdbBuffer(buffer, buf_size, &page, mode_page_size, ret_size);
}
void ReplyModePageInfoExceptionCtrl(
    PSPC_DEVEXT devext,
    PUCHAR& buffer, 
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size)
{
    MODE_INFO_EXCEPTIONS page = { 0 };
    UNREFERENCED_PARAMETER(devext);
    mode_page_size = sizeof(MODE_INFO_EXCEPTIONS);
    FillModePage_InfoException(&page);
    CopyToCdbBuffer(buffer, buf_size, &page, mode_page_size, ret_size);
}

UCHAR ReadWriteRamdisk(PSPC_SRBEXT srbext)
{
    PSPC_DEVEXT devext = srbext->DevExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    if(!srbext->IsScsiWrite)
    {
        status = devext->Read(srbext->RwOffsetBytes, 
                                srbext->RwLengthBytes, 
                                srbext->DataBuf);
    }
    else
    {
        status = devext->Write(srbext->RwOffsetBytes, 
                                srbext->RwLengthBytes, 
                                srbext->DataBuf);
    }

    if (!NT_SUCCESS(status))
        return SRB_STATUS_ERROR;
    return SRB_STATUS_SUCCESS;
}
