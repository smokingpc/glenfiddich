#include "precompile.h"
#if 0
void IoWorkerThreadRoutine(PVOID thread_ctx)
{
    PWORKER_THREAD_CTX ctx = (PWORKER_THREAD_CTX)thread_ctx;
    PSPC_DEVEXT devext = (PSPC_DEVEXT)ctx->DevExt;
    ULONG counter = 0;

    while(STATUS_WAIT_0 != KeWaitForSingleObject(ctx->StopEventPtr,
                Executive, KernelMode, FALSE, &ctx->Interval))
    {
        counter = devext->ProcessIoRequests();
        KdPrintEx((DPFLTR_IHVDRIVER_ID, 1, "WorkerThread processed [%lu] Srb\n", counter));
    }

    devext->AbortIoRequests();
    ctx->IsStopped = true;
}
#endif

void _SPC_DEVEXT::Setup()
{
    RtlStringCchPrintfA((NTSTRSAFE_PSTR)this->SN, SN_STRBUF_SIZE, "%llu", DISK_SN);
    RtlStringCbLengthA((STRSAFE_PCNZCH)this->SN, SN_STRBUF_SIZE, &this->SnStrLen);
    RtlStringCchPrintfA((NTSTRSAFE_PSTR)this->NQN, NQN_BUF_SIZE, NQN_FORMAT, this->SN);
    RtlStringCbLengthA((STRSAFE_PCNZCH)this->NQN, NQN_BUF_SIZE, &this->NqnStrLen);
    LoadDefault();
    LoadRegistry();
    Disk = (PUCHAR) new(NonPagedPoolNx, TAG_DISKMEM) UCHAR[(size_t)TotalDiskBytes];
}
void _SPC_DEVEXT::Teardown()
{
    if(NULL != Disk)
    {
        delete Disk;
        Disk = NULL;
    }
}
BOOLEAN _SPC_DEVEXT::IsValidOffsetAndLength(ULONG_PTR offset, ULONG length)
{
    return ((offset + length) <= TotalDiskBytes);
}
BOOLEAN _SPC_DEVEXT::IsValidLbaAndLength(ULONG_PTR lba_start, ULONG block_count)
{
    return ((lba_start + block_count) <= TotalBlocks);
}
NTSTATUS _SPC_DEVEXT::ReadLBA(ULONG_PTR start_block, ULONG blocks, PVOID buffer)
{
    ULONG_PTR start_offset = start_block * BlockSizeInBytes;
    ULONG read_bytes = blocks * BlockSizeInBytes;

    return Read(start_offset, read_bytes, buffer);
}
NTSTATUS _SPC_DEVEXT::WriteLBA(ULONG_PTR start_block, ULONG blocks, PVOID buffer)
{
    ULONG_PTR start_offset = start_block * BlockSizeInBytes;
    ULONG write_bytes = blocks * BlockSizeInBytes;

    return Write(start_offset, write_bytes, buffer);
}

NTSTATUS _SPC_DEVEXT::Read(ULONG_PTR offset, ULONG length, PVOID buffer)
{
    if (!IsValidOffsetAndLength(offset, length) || (NULL == buffer))
        return STATUS_INVALID_PARAMETER;

    PUCHAR start_va = Disk + offset;
    RtlCopyMemory(buffer, start_va, length);
    return STATUS_SUCCESS;
}
NTSTATUS _SPC_DEVEXT::Write(ULONG_PTR offset, ULONG length, PVOID buffer)
{
    if (!IsValidOffsetAndLength(offset, length) || (NULL == buffer))
        return STATUS_INVALID_PARAMETER;

    PUCHAR start_va = Disk + offset;
    RtlCopyMemory(start_va, buffer, length);
    return STATUS_SUCCESS;
}

#if 0
void _SPC_DEVEXT::PushIoRequest(PVOID arg)
{
    //srbext->List is located at beginning of SPC_SRBEXT.
    //so just force cast this pointer. (lazy way....)
    PSPC_SRBEXT srbext = (PSPC_SRBEXT)arg;
    ExInterlockedPushEntrySList(&RequestHead, &srbext->List, NULL);
}
PVOID _SPC_DEVEXT::PopIoRequest()
{
    return (PSPC_SRBEXT) ExInterlockedPopEntrySList(&RequestHead, NULL);
}
ULONG _SPC_DEVEXT::ProcessIoRequests()
{
    ULONG count = 0;
    for (count = 0; count < WORKER_PROCESS_COUNT; count++)
    {
        ULONG blocks = 0;
        ULONG64 start_block = 0;
        NTSTATUS status = STATUS_SUCCESS;
        UCHAR srb_status = SRB_STATUS_SUCCESS;
        PSPC_SRBEXT srbext = (PSPC_SRBEXT)PopIoRequest();
        if (NULL == srbext)
            break;

        ParseLbaBlockAndOffset(start_block, blocks, srbext->Cdb);
        if(!srbext->IsWrite)
        {
            status = ReadLBA(start_block, blocks, srbext->DataBuf);
        }
        else
        {
            status = WriteLBA(start_block, blocks, srbext->DataBuf);
        }

        if(NT_SUCCESS(status))
            srb_status = SRB_STATUS_SUCCESS;
        else
            srb_status = SRB_STATUS_ERROR;

        srbext->CompleteSrb(srb_status);
    }
    return count;
}
ULONG _SPC_DEVEXT::AbortIoRequests()
{
    ULONG count = 0;
    while (true)
    {
        PSPC_SRBEXT srbext = (PSPC_SRBEXT)PopIoRequest();
        if (NULL == srbext)
            break;

        srbext->CompleteSrb(SRB_STATUS_ABORTED);
        count++;
    }
    return count;
}
void _SPC_DEVEXT::StartWorkerThread()
{
    NTSTATUS status = STATUS_SUCCESS;
    KeInitializeEvent(&EventRequestArrived, SynchronizationEvent, FALSE);
    RtlZeroMemory(WorkerCtx, sizeof(WorkerCtx));
    //StorPortCreateSystemThread is implemented since Win2022.
    //Use PsCreateSystemThread instead of it.
    FlagStopThread = false;
    for(int i=0; i<4; i++)
    {
        WorkerCtx[i].DevExt = this;
        WorkerCtx[i].Interval.QuadPart = WORKER_INTERVAL;
        WorkerCtx[i].IsStopped = false;
        WorkerCtx[i].StopEventPtr = &EventRequestArrived;
        WorkerCtx[i].ThreadHandle = NULL;
        status = PsCreateSystemThread(&WorkerCtx[i].ThreadHandle, 
                THREAD_ALL_ACCESS, NULL, NULL, NULL, 
                IoWorkerThreadRoutine, &WorkerCtx[i]);
        ASSERT(status == STATUS_SUCCESS);
    }
}
void _SPC_DEVEXT::StopWorkerThread()
{
    LARGE_INTEGER timeout = {0};
    timeout.QuadPart = -10 * 1000 * 100;
    FlagStopThread = true;
    for (int i = 0; i < 4; i++)
        KeSetEvent(&EventRequestArrived, IO_NO_INCREMENT, FALSE);

    while(!(WorkerCtx[0].IsStopped && WorkerCtx[1].IsStopped
                && WorkerCtx[2].IsStopped && WorkerCtx[3].IsStopped))
        KeDelayExecutionThread(KernelMode, FALSE, &timeout);
    //StorPortWaitForSingleObject(this, &WorkerThread[0], FALSE, &timeout);
}
#endif

void _SPC_DEVEXT::SetDiskSize(size_t total_bytes, ULONG bytes_of_block)
{
    //todo: check "is bytes_of_block == 512 or 4096?"
    BlockSizeInBytes = bytes_of_block;
    //todo: check "is total_bytes align to bytes_of_block?"
    TotalDiskBytes = total_bytes;
    TotalBlocks = total_bytes / bytes_of_block;
    MaxLBA = TotalBlocks - 1;    //MaxLBA is (0 based) max index of LBA blocks
}
void _SPC_DEVEXT::LoadDefault()
{
    SetDiskSize(DEFAULT_DISK_BYTES, DEFAULT_BLOCK_SIZE);
    ReadCacheEnabled = WriteCacheEnabled = FALSE;
}
void _SPC_DEVEXT::LoadRegistry()
{
    UCHAR* buffer = NULL;
    ULONG size = sizeof(ULONG);
    ULONG size_in_mb = 0;
    ULONG block_size = 0;
    ULONG read_size = 0;
    BOOLEAN ok = FALSE;
    UCHAR* name = NULL;

    buffer = StorPortAllocateRegistryBuffer(this, &size);
    if (NULL == buffer)
        return;

    memset(buffer, 0, size);
    name = const_cast<UCHAR*> (LABEL_DISKSIZE);
    read_size = size;
    ok = StorPortRegistryRead(this, name, TRUE, MINIPORT_REG_DWORD, buffer, &read_size);
    if (ok)
        StorPortCopyMemory(&size_in_mb, buffer, size);

    memset(buffer, 0, size);
    name = const_cast<UCHAR*> (LABEL_BLOCKSIZE);
    read_size = size;
    ok = StorPortRegistryRead(this, name, TRUE, MINIPORT_REG_DWORD, buffer, &read_size);
    if (ok)
        StorPortCopyMemory(&block_size, buffer, size);

    if (0 == size_in_mb)
        size = DEFAULT_DISK_BYTES;
    else
        size = size_in_mb * MegaBytes;

    if (block_size != DEFAULT_BLOCK_SIZE && block_size != DEFAULT_BLOCK_SIZE2)
        block_size = DEFAULT_BLOCK_SIZE;

    SetDiskSize(size, block_size);
    StorPortFreeRegistryBuffer(this, buffer);
}
