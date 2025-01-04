#include "precompile.h"

#if 0
UCHAR Scsi_Read12(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, FALSE);
}
UCHAR Scsi_Write12(PSPC_SRBEXT srbext)
{
    return ReadWriteRamdisk(srbext, TRUE);
}
#endif

UCHAR Scsi_ReportLuns12(PSPC_SRBEXT srbext)
{
    UCHAR srb_status = SRB_STATUS_INVALID_REQUEST;
    UNREFERENCED_PARAMETER(srbext);
    //REPORT_LUNS + LUN_LIST structures
    return srb_status;
}
UCHAR Scsi_Verify12(PSPC_SRBEXT srbext)
{
    //todo: complete this handler for FULL support of verify
    UCHAR srb_status = SRB_STATUS_ERROR;
    PCDB cdb = srbext->Cdb;
    UINT32 lba_start = 0;    //in Blocks, not bytes
    REVERSE_BYTES_4(&lba_start, cdb->CDB12.LogicalBlock);

    UINT32 verify_len = 0;    //in Blocks, not bytes
    REVERSE_BYTES_4(&verify_len, &cdb->CDB12.TransferLength);

    if (FALSE == srbext->DevExt->IsValidLbaAndLength(lba_start, verify_len))
        srb_status = SRB_STATUS_SUCCESS;

    return srb_status;
}

