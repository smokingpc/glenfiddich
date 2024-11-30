#include "precompile.h"

UCHAR Scsi_Read16(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, FALSE);
}

UCHAR Scsi_Write16(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, TRUE);
}

UCHAR Scsi_Verify16(PSPC_SRBEXT srbext)
{
    //todo: complete this handler for FULL support of verify
    UCHAR srb_status = SRB_STATUS_ERROR;
    PCDB cdb = srbext->Cdb;
    UINT64 lba_start = 0;    //in Blocks, not bytes
    
    REVERSE_BYTES_8(&lba_start, cdb->CDB16.LogicalBlock);

    UINT32 verify_len = 0;    //in Blocks, not bytes
    REVERSE_BYTES_4(&verify_len, &cdb->CDB16.TransferLength);

    if (FALSE == srbext->DevExt->IsValidLbaAndLength(lba_start, verify_len))
        srb_status = SRB_STATUS_SUCCESS;

    SrbSetDataTransferLength(srbext->Srb, 0);
    return srb_status;
}

UCHAR Scsi_ReadCapacity16(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_SUCCESS;
    ULONG ret_size = sizeof(READ_CAPACITY16_DATA);
    PREAD_CAPACITY16_DATA readcap = (PREAD_CAPACITY16_DATA)srbext->DataBuf;
    RtlZeroMemory(readcap, srbext->DataBufLen);

    if(srbext->DataBufLen >= sizeof(READ_CAPACITY16_DATA))
    {
        ULONG block_size = srbext->DevExt->BlockSizeInBytes;
        INT64 last_lba = srbext->DevExt->MaxLBA;

        REVERSE_BYTES_QUAD(&readcap->LogicalBlockAddress, &last_lba);
        REVERSE_BYTES(&readcap->BytesPerBlock, &block_size);
        readcap->ProtectionEnable = FALSE;
        //todo: fill other fields in READ_CAPACITY16_DATA
        //e.g.      UCHAR ProtectionEnable : 1;
        //          UCHAR ProtectionType : 3;
    }
    else
    {
        srb_status = SRB_STATUS_DATA_OVERRUN;
    }

    UpdateDataBufLen(srbext, ret_size);
    return srb_status;
}

UCHAR Scsi_SynchronizeCache16(PSPC_SRBEXT srbext)
{
    UNREFERENCED_PARAMETER(srbext);
    return SRB_STATUS_SUCCESS;
}
