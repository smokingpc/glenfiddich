#include "precompile.h"

static __inline bool IsStorageRequestBlock(
    _In_ PSCSI_REQUEST_BLOCK srb)
{
    return (SRB_FUNCTION_STORAGE_REQUEST_BLOCK == srb->Function);
}
#if 0
static __inline bool IsScsiWrite(UCHAR opcode)
{
     return (SCSIOP_WRITE6 == opcode) ||
         (SCSIOP_WRITE == opcode)||
         (SCSIOP_WRITE12 == opcode)||
         (SCSIOP_WRITE16 == opcode);
}

static __inline bool IsScsiReadWrite(UCHAR opcode)
{
    switch (opcode)
    {
    case SCSIOP_READ6:
    case SCSIOP_READ:
    case SCSIOP_READ12:
    case SCSIOP_READ16:
    case SCSIOP_WRITE6:
    case SCSIOP_WRITE:
    case SCSIOP_WRITE12:
    case SCSIOP_WRITE16:
        return TRUE;
    }

    return FALSE;
}
#endif

static void ParseStorportAddr(_In_ PSPC_SRBEXT srbext)
{
    PSCSI_REQUEST_BLOCK srb = srbext->Srb;
    if (IsStorageRequestBlock(srb))
    {
        PSTOR_ADDR_BTL8 addr =
            (PSTOR_ADDR_BTL8)SrbGetAddress((PSTORAGE_REQUEST_BLOCK)srb);
        srbext->Bus = addr->Path;
        srbext->Target = addr->Target;
        srbext->Lun = addr->Lun;
        srbext->RaidPort = addr->Port;   //miniport RaidPortXX number(saw in windbg). determined by storport.
    }
    else
    {
        srbext->Bus = srb->PathId;
        srbext->Target = srb->TargetId;
        srbext->Lun = srb->Lun;
        srbext->RaidPort = INVALID_RAIDPORT;
    }
}

static void UpdateScsiStateToSrb(
    _In_ PSPC_SRBEXT srbext,
    _Inout_ UCHAR &srb_status)
{
    if (nullptr == srbext->Srb)
        return;
    PSENSE_DATA sdata = (PSENSE_DATA)SrbGetSenseInfoBuffer(srbext->Srb);
    UCHAR sdata_size = SrbGetSenseInfoBufferLength(srbext->Srb);

    //do nothing for SRB_STATUS_PENDING.
    //Don't set scsistate for PENDING.
    switch (srb_status)
    {
    case SRB_STATUS_SUCCESS:
        SrbSetScsiStatus(srbext->Srb, SCSISTAT_GOOD);
        break;
    case SRB_STATUS_PENDING:
        break;
    case SRB_STATUS_BUSY:
        SrbSetScsiStatus(srbext->Srb, SCSISTAT_BUSY);
        break;
    default:
        srb_status = srb_status | SRB_STATUS_AUTOSENSE_VALID;
        if (NULL == sdata || 0 == sdata_size)
        {
            SrbSetScsiStatus(srbext->Srb, SCSISTAT_CONDITION_MET);
        }
        else
        {
            RtlZeroMemory(sdata, sdata_size);
            sdata->ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            sdata->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
            sdata->AdditionalSenseLength = sdata_size - FIELD_OFFSET(SENSE_DATA, AdditionalSenseLength);
            sdata->AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_COMMAND;
            sdata->AdditionalSenseCodeQualifier = 0;
            SrbSetScsiStatus(srbext->Srb, SCSISTAT_CHECK_CONDITION);
        }
        break;
    }
}

static void ParseScsiReadWriteLBA(_In_ PSPC_SRBEXT srbext)
{
    PCDB cdb = srbext->Cdb;
    UCHAR opcode = cdb->CDB6GENERIC.OperationCode;
    PUCHAR cursor = nullptr;

    switch (opcode)
    {
    case SCSIOP_READ6:
    case SCSIOP_WRITE6:
    {
        cursor = (PUCHAR)srbext->RwOffset;
        cursor[0] = cdb->CDB6READWRITE.LogicalBlockLsb;
        cursor[1] = cdb->CDB6READWRITE.LogicalBlockMsb0;
        cursor[2] = cdb->CDB6READWRITE.LogicalBlockMsb1;
        srbext->RwLength = 
            cdb->CDB6READWRITE.TransferBlocks ? cdb->CDB6READWRITE.TransferBlocks : 256;
        break;
    }
    case SCSIOP_READ:
    case SCSIOP_WRITE:
    case SCSIOP_VERIFY:
    {
        REVERSE_BYTES_4(srbext->RwOffset, &cdb->CDB10.LogicalBlockByte0);
        REVERSE_BYTES_2(srbext->RwLength, &cdb->CDB10.TransferBlocksMsb);
        break;
    }
    case SCSIOP_READ12:
    case SCSIOP_WRITE12:
    case SCSIOP_VERIFY12:
    {
        REVERSE_BYTES_4(srbext->RwOffset, &cdb->CDB12.LogicalBlock);
        REVERSE_BYTES_4(srbext->RwLength, &cdb->CDB12.TransferLength);
        break;
    }
    case SCSIOP_READ16:
    case SCSIOP_WRITE16:
    case SCSIOP_VERIFY16:
    {
        REVERSE_BYTES_8(srbext->RwOffset, cdb->CDB16.LogicalBlock);
        REVERSE_BYTES_4(srbext->RwLength, cdb->CDB16.TransferLength);
        break;
    }

    default:
        srbext->RwOffset = 0;
        srbext->RwLength = 0;
        break;
    }

    srbext->RwOffsetBytes = srbext->RwOffset * srbext->DevExt->BlockSizeInBytes;
    srbext->RwLengthBytes = srbext->RwLength * srbext->DevExt->BlockSizeInBytes;
}

static void ParseScsiInfoFromSrb(_In_ PSPC_SRBEXT srbext)
{
    PSCSI_REQUEST_BLOCK srb = srbext->Srb;
    srbext->Cdb = SrbGetCdb(srb);
    srbext->CdbLen = SrbGetCdbLength(srb);
    srbext->FuncCode = SrbGetSrbFunction(srb);
    srbext->DataBuf = SrbGetDataBuffer(srb);
    srbext->DataBufLen = SrbGetDataTransferLength(srb);
    srbext->ScsiTag = SrbGetQueueTag(srb);

    ParseScsiReadWriteLBA(srbext);
}

static void InitSrbext(
    _In_ PSPC_SRBEXT srbext,
    _In_ PSCSI_REQUEST_BLOCK srb,
    _In_ PVOID devext)
{
    RtlZeroMemory(srbext, sizeof(SPC_SRBEXT));
    srbext->Srb = srb;
    srbext->DevExt = (PSPC_DEVEXT)devext;
    srbext->Protect = OVERRUN_TAG;
    srbext->Bus = INVALID_BUS_ID;
    srbext->Target = INVALID_TARGET_ID;
    srbext->Lun = INVALID_LUN_ID;
    srbext->RaidPort = INVALID_RAIDPORT;
    srbext->ScsiTag = INVALID_SCSI_TAG;
    if (nullptr != srbext->Srb)
    {
        ParseScsiInfoFromSrb(srbext);
        ParseStorportAddr(srbext);
    }
}

void CompleteSrb(_In_ PSPC_SRBEXT srbext, _In_ UCHAR srb_status)
{
    if (nullptr != srbext->Srb)
    {
        UpdateScsiStateToSrb(srbext, srb_status);
        SrbSetSrbStatus(srbext->Srb, srb_status);
        StorPortNotification(RequestComplete, srbext->DevExt, srbext->Srb);
    }
}
void UpdateDataBufLen(_In_ PSPC_SRBEXT srbext, _In_ ULONG len)
{
    if (nullptr != srbext->Srb)
        SrbSetDataTransferLength(srbext->Srb, len);
    srbext->DataBufLen = len;
}
bool GetSrbPnpRequest(
    _In_ PSPC_SRBEXT srbext, 
    _Inout_ ULONG& flags, 
    _Inout_ STOR_PNP_ACTION& action)
{
    if(nullptr == srbext->Srb)
        return false;

//SrbGetSrbExDataByType() return NULL if srb is SCSI_REQUEST_BLOCK
    PSRBEX_DATA_PNP pnp = (PSRBEX_DATA_PNP)SrbGetSrbExDataByType(
        (PSTORAGE_REQUEST_BLOCK)srbext->Srb, SrbExDataTypePnP);

    if (pnp != nullptr) {
        flags = pnp->SrbPnPFlags;
        action = pnp->PnPAction;
    }
    else {
        PSCSI_PNP_REQUEST_BLOCK scsi_pnp = 
            (PSCSI_PNP_REQUEST_BLOCK)srbext->Srb;
        flags = scsi_pnp->SrbPnPFlags;
        action = scsi_pnp->PnPAction;
    }
    return true;
}

PSPC_SRBEXT GetSrbExt(_In_ PSCSI_REQUEST_BLOCK srb, _In_ PVOID devext)
{
    PSPC_SRBEXT srbext = (PSPC_SRBEXT)SrbGetMiniportContext(srb);
    InitSrbext(srbext, srb, devext);
    return srbext;
}
