#include "precompile.h"

typedef struct _CDB6_REQUESTSENSE
{
    UCHAR OperationCode;
    UCHAR DescFormat : 1;
    UCHAR Reserved1 : 7;
    UCHAR Reserved2[2];
    UCHAR AllocSize;
    struct {
        UCHAR Link : 1;         //obsoleted by T10
        UCHAR Flag : 1;         //obsoleted by T10
        UCHAR NormalACA : 1;
        UCHAR Reserved : 3;
        UCHAR VenderSpecific : 2;
    }Control;
}CDB6_REQUESTSENSE, *PCDB6_REQUESTSENSE;

#pragma region ======== static functions ========
static UCHAR Reply_VpdSupportedPages(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    UCHAR supported_pages = 5;
    ULONG buf_size = 0;
    PVPD_SUPPORTED_PAGES_PAGE page = NULL;
    ULONG stor_status = STOR_STATUS_SUCCESS;

    buf_size = (FIELD_OFFSET(VPD_SUPPORTED_PAGES_PAGE, SupportedPageList) + supported_pages);
    stor_status = StorPortAllocatePool(srbext->DevExt, buf_size, TAG_VPDPAGE, (PVOID*)&page);

    if (stor_status != STOR_STATUS_SUCCESS)
        return SRB_STATUS_INTERNAL_ERROR;

    RtlZeroMemory(page, buf_size);
    page->DeviceType = DIRECT_ACCESS_DEVICE;
    page->DeviceTypeQualifier = DEVICE_CONNECTED;
    page->PageCode = VPD_SUPPORTED_PAGES;
    page->PageLength = supported_pages;
    page->SupportedPageList[0] = VPD_SUPPORTED_PAGES;
    page->SupportedPageList[1] = VPD_SERIAL_NUMBER;
    page->SupportedPageList[2] = VPD_DEVICE_IDENTIFIERS;
    page->SupportedPageList[3] = VPD_BLOCK_LIMITS;
    page->SupportedPageList[4] = VPD_BLOCK_DEVICE_CHARACTERISTICS;

    ULONG data_len = min(srbext->DataBufLen, buf_size);
    StorPortCopyMemory(srbext->DataBuf, page, data_len);
    ret_size = data_len;
    StorPortFreePool(srbext->DevExt, page);
    return SRB_STATUS_SUCCESS;
}

static UCHAR Reply_VpdSerialNumber(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    PSPC_DEVEXT devext = srbext->DevExt;
    PUCHAR buffer = (PUCHAR)srbext->DataBuf;
    ULONG buf_size = srbext->DataBufLen;
    size_t size = devext->SnStrLen;
    ret_size = (ULONG)(size + sizeof(VPD_SERIAL_NUMBER_PAGE) + 1);

    if (ret_size <= buf_size)
    {
        PVPD_SERIAL_NUMBER_PAGE page = (PVPD_SERIAL_NUMBER_PAGE)buffer;
        RtlZeroMemory(buffer, buf_size);
        page->DeviceType = DIRECT_ACCESS_DEVICE;
        page->DeviceTypeQualifier = DEVICE_CONNECTED;
        page->PageCode = VPD_SERIAL_NUMBER;
        page->PageLength = (UCHAR)devext->SnStrLen;
        page++;
        memcpy(page, devext->SN, (UCHAR)devext->SnStrLen);

        return SRB_STATUS_SUCCESS;
    }
    return SRB_STATUS_DATA_OVERRUN;
}

static UCHAR Reply_VpdDeviceIdentifier(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    PSPC_DEVEXT devext = srbext->DevExt;
    PVPD_IDENTIFICATION_PAGE page = NULL;
    ULONG buf_size = 0;
    ULONG sn_size = (ULONG)devext->SnStrLen;
    ULONG vid_size = (ULONG)strlen(VENDOR_ID);
    ULONG stor_status = STOR_STATUS_SUCCESS;
    buf_size = (ULONG)sizeof(VPD_IDENTIFICATION_PAGE) + 
                (ULONG)sizeof(VPD_IDENTIFICATION_DESCRIPTOR) + 
                sn_size + vid_size + 2;

    stor_status = StorPortAllocatePool(devext, buf_size, TAG_VPDPAGE, (PVOID*)&page);
    if (stor_status != STOR_STATUS_SUCCESS)
        return SRB_STATUS_INTERNAL_ERROR;

    RtlZeroMemory(page, buf_size);

    //NQN is too long. So only use VID + SN as Identifier.
    PVPD_IDENTIFICATION_DESCRIPTOR desc = NULL;
    ULONG size = sizeof(VPD_IDENTIFICATION_DESCRIPTOR) + vid_size + sn_size + 2;
    page->DeviceType = DIRECT_ACCESS_DEVICE;
    page->DeviceTypeQualifier = DEVICE_CONNECTED;
    page->PageCode = VPD_DEVICE_IDENTIFIERS;
    page->PageLength = (UCHAR)size;
    desc = (PVPD_IDENTIFICATION_DESCRIPTOR)page->Descriptors;

    desc->CodeSet = VpdCodeSetAscii;
    desc->IdentifierType = VpdIdentifierTypeVendorId;
    desc->Association = VpdAssocDevice;
    desc->IdentifierLength = (UCHAR)(sn_size + vid_size);
    StorPortCopyMemory(desc->Identifier, VENDOR_ID, vid_size);
    StorPortCopyMemory(&desc->Identifier[vid_size], "_", 1);
    StorPortCopyMemory(&desc->Identifier[vid_size+1], devext->SN, sn_size);

    ULONG data_len = min(srbext->DataBufLen, buf_size);
    StorPortCopyMemory(srbext->DataBuf, page, data_len);
    StorPortFreePool(srbext->DevExt, page);
    ret_size = data_len;

    return SRB_STATUS_SUCCESS;
}

static UCHAR Reply_VpdBlockLimits(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    //Max SCSI transfer block size
    //question : is it really used in modern windows system? Orz
    PSPC_DEVEXT devext = srbext->DevExt;
    PVPD_BLOCK_LIMITS_PAGE page = NULL;
    ULONG buf_size = sizeof(VPD_BLOCK_LIMITS_PAGE);
    ULONG stor_status = STOR_STATUS_SUCCESS;
    
    stor_status = StorPortAllocatePool(devext, buf_size, TAG_VPDPAGE, (PVOID*)&page);
    if (stor_status != STOR_STATUS_SUCCESS)
        return SRB_STATUS_INTERNAL_ERROR;

    RtlZeroMemory(page, buf_size);

    page->DeviceType = DIRECT_ACCESS_DEVICE;
    page->DeviceTypeQualifier = DEVICE_CONNECTED;
    page->PageCode = VPD_BLOCK_LIMITS;
    REVERSE_BYTES_2(page->PageLength, &buf_size);

    ULONG max_tx_blocks = MAX_TX_SIZE / devext->BlockSizeInBytes;   //size in LBA blocks
    //tell I/O system: max tx blocks and optimal tx blocks of this adapter.
    REVERSE_BYTES_4(page->MaximumTransferLength, &max_tx_blocks);
    REVERSE_BYTES_4(page->OptimalTransferLength, &max_tx_blocks);

    //Refer to SCSI SBC3 doc or SCSI reference Block Limits VPD page.
    //http://www.13thmonkey.org/documentation/SCSI/sbc3r25.pdf
    USHORT granularity = 0;
    REVERSE_BYTES_2(page->OptimalTransferLengthGranularity, &granularity);

    ULONG data_size = min(srbext->DataBufLen, buf_size);
    StorPortCopyMemory(srbext->DataBuf, page, data_size);
    ret_size = data_size;
    StorPortFreePool(srbext->DevExt, page);

    return SRB_STATUS_SUCCESS;
}

static UCHAR Reply_VpdBlockDeviceCharacteristics(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    PSPC_DEVEXT devext = srbext->DevExt;
    PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE page = NULL;
    ULONG buf_size = sizeof(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE);
    ULONG stor_status = STOR_STATUS_SUCCESS;

    stor_status = StorPortAllocatePool(devext, buf_size, TAG_VPDPAGE, (PVOID*)&page);
    if (stor_status != STOR_STATUS_SUCCESS)
        return SRB_STATUS_INTERNAL_ERROR;

    RtlZeroMemory(page, buf_size);
    page->DeviceType = DIRECT_ACCESS_DEVICE;
    page->DeviceTypeQualifier = DEVICE_CONNECTED;
    page->PageCode = VPD_BLOCK_DEVICE_CHARACTERISTICS;
    page->PageLength = (UCHAR)buf_size;
    page->MediumRotationRateLsb = 1;
    page->NominalFormFactor = 0;

    ULONG data_size = min(srbext->DataBufLen, buf_size);
    StorPortCopyMemory(srbext->DataBuf, page, data_size);
    ret_size = data_size;
    StorPortFreePool(srbext->DevExt, page);

    return SRB_STATUS_SUCCESS;
}

static UCHAR Reply_VpdInquiry(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    PCDB cdb = srbext->Cdb;
    UCHAR srb_status = SRB_STATUS_INVALID_REQUEST;
    switch (cdb->CDB6INQUIRY.PageCode)
    {
    case VPD_SUPPORTED_PAGES:
        srb_status = Reply_VpdSupportedPages(srbext, ret_size);
        break;
    case VPD_SERIAL_NUMBER:
        srb_status = Reply_VpdSerialNumber(srbext, ret_size);
        break;
    case VPD_DEVICE_IDENTIFIERS:
        srb_status = Reply_VpdDeviceIdentifier(srbext, ret_size);
        break;
    case VPD_BLOCK_LIMITS:
        srb_status = Reply_VpdBlockLimits(srbext, ret_size);
        break;
    case VPD_BLOCK_DEVICE_CHARACTERISTICS:
        srb_status = Reply_VpdBlockDeviceCharacteristics(srbext, ret_size);
        break;
    default:
        srb_status = SRB_STATUS_INVALID_REQUEST;
        break;
    }

    return srb_status;
}

static void Fill_InquiryData(PINQUIRYDATA data, char* vid, char* pid, char* rev)
{
    data->DeviceType = DIRECT_ACCESS_DEVICE;
    data->DeviceTypeQualifier = DEVICE_CONNECTED;
    data->RemovableMedia = 0;
    data->Versions = 0x06;
    data->NormACA = 0;
    data->HiSupport = 0;
    data->ResponseDataFormat = 2;
    data->AdditionalLength = INQUIRYDATABUFFERSIZE - 5;  // Amount of data we are returning
    data->EnclosureServices = 0;
    data->MediumChanger = 0;
    data->CommandQueue = 1;
    data->Wide16Bit = 0;
    data->Addr16 = 0;
    data->Synchronous = 0;
    data->Reserved3[0] = 0;

    data->Wide32Bit = TRUE;
    data->LinkedCommands = FALSE;   // No Linked Commands
    RtlCopyMemory((PUCHAR)&data->VendorId[0], vid, strlen(vid));
    RtlCopyMemory((PUCHAR)&data->ProductId[0], pid, strlen(pid));
    RtlCopyMemory((PUCHAR)&data->ProductRevisionLevel[0], rev, strlen(rev));
}

static UCHAR Reply_NonVpdInquiry(PSPC_SRBEXT srbext, ULONG& ret_size)
{
    PINQUIRYDATA data = (PINQUIRYDATA)srbext->DataBuf;
    UCHAR srb_status = SRB_STATUS_DATA_OVERRUN;
    ULONG size = srbext->DataBufLen;
    ret_size = INQUIRYDATABUFFERSIZE;
    if (size >= INQUIRYDATABUFFERSIZE)
    {
        RtlZeroMemory(srbext->DataBuf, srbext->DataBufLen);
        Fill_InquiryData(data, (char*)VENDOR_ID, (char*)PRODUCT_ID, REV_ID);
        srb_status = SRB_STATUS_SUCCESS;
    }
    return srb_status;
}

#pragma endregion

#if 0
UCHAR Scsi_Read6(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, srbext->IsScsiWrite);
}
UCHAR Scsi_Write6(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, TRUE);
}
#endif
UCHAR Scsi_RequestSense6(PSPC_SRBEXT srbext)
{
    //request sense is used to query error information from device.
    //device should reply SENSE_DATA structure.
    //Todo: return real sense data back....
    UCHAR srb_status = SRB_STATUS_ERROR;
    PCDB cdb = srbext->Cdb;
    PCDB6_REQUESTSENSE request = (PCDB6_REQUESTSENSE) cdb->AsByte;
    UINT32 alloc_size = request->AllocSize;     //cdb->CDB6GENERIC.CommandUniqueBytes[2];
    UINT8 format = request->DescFormat;   //cdb->CDB6GENERIC.Immediate;

    ULONG copy_size = 0;

    //DescFormat field indicates "which format of sense data should be returned?"
    //1 == descriptor format sense data shall be returned. (desc header + multiple SENSE_DATA)
    //0 == return fixed format data (just return one SENSE_DATA structure)
    SENSE_DATA_EX data = {0};
    if (1 == format)
    {
        data.DescriptorData.ErrorCode = SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT;
        data.DescriptorData.SenseKey = SCSI_SENSE_NO_SENSE;
        data.DescriptorData.AdditionalSenseCode = SCSI_ADSENSE_NO_SENSE;
        data.DescriptorData.AdditionalSenseCodeQualifier = 0;
        data.DescriptorData.AdditionalSenseLength = 0;
        srb_status = SRB_STATUS_SUCCESS;
        copy_size = sizeof(DESCRIPTOR_SENSE_DATA);
    }
    else
    {
        /* Fixed Format Sense Data */
        data.FixedData.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
        data.FixedData.SenseKey = SCSI_SENSE_NO_SENSE;
        data.FixedData.AdditionalSenseCode = SCSI_ADSENSE_NO_SENSE;
        data.FixedData.AdditionalSenseCodeQualifier = 0;
        data.FixedData.AdditionalSenseLength = 0;

        srb_status = SRB_STATUS_SUCCESS;
        copy_size = sizeof(SENSE_DATA);
    }

    if (copy_size > alloc_size)
        copy_size = alloc_size;

    StorPortCopyMemory(srbext->DataBuf, &data, copy_size);
    srbext->SetSrbDataTxLen(copy_size);
    return srb_status;
}
UCHAR Scsi_Inquiry6(PSPC_SRBEXT srbext) 
{
    UCHAR srb_status = SRB_STATUS_INVALID_REQUEST;
    ULONG ret_size = 0;
    PCDB cdb = srbext->Cdb;

    if (cdb->CDB6INQUIRY3.EnableVitalProductData)
    {
        srb_status = Reply_VpdInquiry(srbext, ret_size);
    }
    else
    {
        if (cdb->CDB6INQUIRY3.PageCode > 0) 
        {
            srb_status = SRB_STATUS_ERROR;
        }
        else
        {
            srb_status = Reply_NonVpdInquiry(srbext, ret_size);
        }
    }
    
    srbext->SetSrbDataTxLen(ret_size);
    return srb_status;
}
UCHAR Scsi_Verify6(PSPC_SRBEXT srbext)
{
//VERIFY(6) seems obsoleted? I didn't see description in Seagate SCSI reference.
    UCHAR srb_status = SRB_STATUS_INVALID_REQUEST;
    UNREFERENCED_PARAMETER(srbext);
    return srb_status;
}
UCHAR Scsi_ModeSelect6(PSPC_SRBEXT srbext)
{
//In DeviceManager GUI, Property Page of each disk can select write-cache policy.
//It is implemented by ModeSelect6 cmd.
    if (srbext->DataBufLen < sizeof(MODE_PARAMETER_HEADER)
        || srbext->DataBuf == nullptr)
    {
        //ScsiHandler should set DataTxLength to tell upper : 
        //    I returned data with length xxx.
        //some command will fail if not set this data transfer length.
        srbext->SetSrbDataTxLen(sizeof(MODE_PARAMETER_HEADER));
        return SRB_STATUS_ERROR;
    }

    CDB::_MODE_SELECT* select = &srbext->Cdb->MODE_SELECT;
    PMODE_PARAMETER_HEADER header = (PMODE_PARAMETER_HEADER)srbext->DataBuf;
    PMODE_PARAMETER_BLOCK param_block = (PMODE_PARAMETER_BLOCK)(header + 1);
    PUCHAR cursor = ((PUCHAR)param_block + header->BlockDescriptorLength);
    ULONG mode_data_size = 0;
    ULONG offset = 0;

    //currently don't support VendorSpecific MODE_SELECT op.
    if (0 == select->PFBit)
        return SRB_STATUS_INVALID_REQUEST;

    //if BlockDescriptorLength is zero, data following param_block are
    //all mode_pages to set into SCSI. e.g. MODE_CACHE_PAGE...etc...
    if (0 == header->BlockDescriptorLength)
        param_block = nullptr;

    //windows set header->ModeDataLength to 0. No idea if it is bug or lazy....
    mode_data_size = srbext->DataBufLen -
        sizeof(MODE_PARAMETER_HEADER) -
        header->BlockDescriptorLength;

    while (mode_data_size > 0)
    {
        PMODE_CACHING_PAGE page = (PMODE_CACHING_PAGE)(cursor + offset);
        mode_data_size -= (page->PageLength + 2);
        offset += (page->PageLength + 2);

        if (0 == page->PageLength)
            break;

        //Only handle MODE_PAGE_CACHING now. all other pages not supported.
        //todo: support all MODE_PAGE_*
        if (page->PageCode != MODE_PAGE_CACHING || 
                page->PageLength != (sizeof(MODE_CACHING_PAGE) - 2))
            continue;

        //ReadCache is not supported now.
        //Here is just a fake update. it is useless.
        srbext->DevExt->ReadCacheEnabled = !page->ReadDisableCache;
        srbext->DevExt->WriteCacheEnabled = page->WriteCacheEnable;
        break;
    }

    return SRB_STATUS_SUCCESS;
}
UCHAR Scsi_ModeSense6(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_ERROR;
    PUCHAR buffer = (PUCHAR) srbext->DataBuf;
    PMODE_PARAMETER_HEADER header = (PMODE_PARAMETER_HEADER)buffer;
    ULONG buf_size = srbext->DataBufLen;
    ULONG ret_size = 0;
    ULONG page_size = 0;

    if (NULL == buffer || 0 == buf_size)
        return SRB_STATUS_ERROR;

    if (buf_size < sizeof(MODE_PARAMETER_HEADER))
    {
        srb_status = SRB_STATUS_DATA_OVERRUN;
        ret_size = sizeof(MODE_PARAMETER_HEADER);
        goto end;
    }

    FillParamHeader(header);
    buffer += sizeof(MODE_PARAMETER_HEADER);
    buf_size -= sizeof(MODE_PARAMETER_HEADER);
    ret_size += sizeof(MODE_PARAMETER_HEADER);

    // Todo: reply real mode sense data
    switch (srbext->Cdb->MODE_SENSE.PageCode)
    {
        case MODE_PAGE_CACHING:
        {
            ReplyModePageCaching(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            header->ModeDataLength += (UCHAR)page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_PAGE_CONTROL:
        {
            ReplyModePageControl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            header->ModeDataLength += (UCHAR)page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_PAGE_FAULT_REPORTING:
        {
            //in HLK, it required "Information Exception Control Page".
            //But it is renamed to MODE_PAGE_FAULT_REPORTING in Windows Storport ....
            //refet to https://www.t10.org/ftp/t10/document.94/94-190r3.pdf
            ReplyModePageInfoExceptionCtrl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            header->ModeDataLength += (UCHAR)page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_SENSE_RETURN_ALL:
        {
            if(buf_size > 0)
            {
                ReplyModePageCaching(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                header->ModeDataLength += (UCHAR)page_size;
            }
            if (buf_size > 0)
            {
                ReplyModePageControl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                header->ModeDataLength += (UCHAR)page_size;
            }
            if (buf_size > 0)
            {
                ReplyModePageInfoExceptionCtrl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                header->ModeDataLength += (UCHAR)page_size;
            }

            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        default:
        {
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
    }

end:
    srbext->SetSrbDataTxLen(ret_size);
    return srb_status;
}
