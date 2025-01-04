#include "precompile.h"

#if 0
UCHAR Scsi_Read10(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, FALSE);
}
UCHAR Scsi_Write10(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, TRUE);
}
#endif

UCHAR Scsi_ReadCapacity10(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_SUCCESS;
    ULONG ret_size = 0;
    RtlZeroMemory(srbext->DataBuf, srbext->DataBufLen);
    ULONG block_size = srbext->DevExt->BlockSizeInBytes;
    INT64 last_lba = srbext->DevExt->MaxLBA;

    if (srbext->DataBufLen >= sizeof(READ_CAPACITY_DATA_EX))
    {
        PREAD_CAPACITY_DATA_EX readcap = (PREAD_CAPACITY_DATA_EX)srbext->DataBuf;
        REVERSE_BYTES_QUAD(&readcap->LogicalBlockAddress.QuadPart, &last_lba);
        REVERSE_BYTES(&readcap->BytesPerBlock, &block_size);
        ret_size = sizeof(READ_CAPACITY_DATA_EX);
    }
    else if (srbext->DataBufLen >= sizeof(READ_CAPACITY_DATA))
    {
        PREAD_CAPACITY_DATA readcap = (PREAD_CAPACITY_DATA)srbext->DataBuf;

        if (last_lba > 0xFFFFFFFF)
            last_lba = 0xFFFFFFFF;
        REVERSE_BYTES(&readcap->LogicalBlockAddress, &last_lba);
        REVERSE_BYTES(&readcap->BytesPerBlock, &block_size);
        ret_size = sizeof(READ_CAPACITY_DATA);
    }
    else 
    {
        srb_status = SRB_STATUS_DATA_OVERRUN;
        ret_size = sizeof(READ_CAPACITY_DATA_EX);
    }
    srbext->SetSrbDataTxLen(ret_size);
    return srb_status;
}
UCHAR Scsi_Verify10(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_ERROR;
    PCDB cdb = srbext->Cdb;
    UINT32 lba_start = 0;    //in Blocks, not bytes
    REVERSE_BYTES_4(&lba_start, &cdb->CDB10.LogicalBlockByte0);
    
    UINT16 verify_len = 0;    //in Blocks, not bytes
    REVERSE_BYTES_2(&verify_len, &cdb->CDB10.TransferBlocksMsb);

    if(FALSE == srbext->DevExt->IsValidLbaAndLength(lba_start, verify_len))
        srb_status = SRB_STATUS_SUCCESS;

    return srb_status;
}
UCHAR Scsi_ModeSelect10(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_INVALID_REQUEST;
    UNREFERENCED_PARAMETER(srbext);
    return srb_status;
}
UCHAR Scsi_ModeSense10(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_ERROR;
    PUCHAR buffer = (PUCHAR)srbext->DataBuf;
    PMODE_PARAMETER_HEADER10 header = (PMODE_PARAMETER_HEADER10)buffer;
    ULONG buf_size = srbext->DataBufLen;
    ULONG ret_size = 0;
    ULONG page_size = 0;
    ULONG mode_data_size = 0;

    if (NULL == buffer || 0 == buf_size)
        return SRB_STATUS_ERROR;

    if (buf_size < sizeof(MODE_PARAMETER_HEADER10))
    {
        srb_status = SRB_STATUS_DATA_OVERRUN;
        ret_size = sizeof(MODE_PARAMETER_HEADER10);
        goto end;
    }

    FillParamHeader10(header);
    buffer += sizeof(MODE_PARAMETER_HEADER10);
    buf_size -= sizeof(MODE_PARAMETER_HEADER10);
    ret_size += sizeof(MODE_PARAMETER_HEADER10);
    REVERSE_BYTES_2(&mode_data_size, header->ModeDataLength);

    // Todo: reply real mode sense data
    switch (srbext->Cdb->MODE_SENSE.PageCode)
    {
        case MODE_PAGE_CACHING:
        {
            ReplyModePageCaching(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            mode_data_size += page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_PAGE_CONTROL:
        {
            ReplyModePageControl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            mode_data_size += page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_PAGE_FAULT_REPORTING:
        {
            //in HLK, it required "Information Exception Control Page".
            //But it is renamed to MODE_PAGE_FAULT_REPORTING in Windows Storport ....
            //refet to https://www.t10.org/ftp/t10/document.94/94-190r3.pdf
            ReplyModePageInfoExceptionCtrl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
            mode_data_size += page_size;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        case MODE_SENSE_RETURN_ALL:
        {
            if (buf_size > 0)
            {
                ReplyModePageCaching(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                mode_data_size += page_size;
            }
            if (buf_size > 0)
            {
                ReplyModePageControl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                mode_data_size += page_size;
            }
            if (buf_size > 0)
            {
                ReplyModePageInfoExceptionCtrl(srbext->DevExt, buffer, buf_size, page_size, ret_size);
                mode_data_size += page_size;
            }

            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
        default:
        {
            page_size = 0;
            srb_status = SRB_STATUS_SUCCESS;
            break;
        }
    }
    REVERSE_BYTES_2(header->ModeDataLength, &mode_data_size);

end:
    srbext->SetSrbDataTxLen(ret_size);
    return srb_status;
}
UCHAR Scsi_SynchronizeCache10(PSPC_SRBEXT srbext)
{
    UNREFERENCED_PARAMETER(srbext);
    return SRB_STATUS_SUCCESS;
}
