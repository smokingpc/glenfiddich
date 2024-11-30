#pragma once
// ================================================================
// SpcRamdisk : OpenSource Ramdisk Driver for Windows 8+
// Author : Roy Wang(SmokingPC).
// Licensed by MIT License.
// 
// Copyright (C) 2023, Roy Wang (SmokingPC)
// https://github.com/smokingpc/
// 
// Contact Me : smokingpc@gmail.com
// ================================================================
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this softwareand associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and /or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
// IN THE SOFTWARE.
// ================================================================
// This driver is my early exercise for virtual miniport driver.
// It is used to trace miniport callback and SCSI cmds of windows.
// Please keep my name in source code if you use it.
// 
// Enjoy it.
// ================================================================

typedef struct _WORKER_THREAD_CTX {
    HANDLE ThreadHandle;
    PVOID DevExt;
    bool IsStopped;
    KEVENT *StopEventPtr;
    LARGE_INTEGER Interval;
}WORKER_THREAD_CTX, *PWORKER_THREAD_CTX;

//DeviceExtension for Ramdisk, this is "per instance" context
typedef struct _SPC_DEVEXT {
    static constexpr UCHAR LABEL_DISKSIZE[] = "DiskSize";
    static constexpr UCHAR LABEL_BLOCKSIZE[] = "BlockSize";

    PDEVICE_OBJECT  FDO;             //my device object
    PDEVICE_OBJECT  PDO;            //Physical device object
    PDEVICE_OBJECT  LowerDO;        //lower level device object(usually lower filter or pdo)
    PDRIVER_OBJECT  Driver;

    UCHAR SN[SN_STRBUF_SIZE];
    size_t SnStrLen;
    UCHAR NQN[NQN_BUF_SIZE];
    size_t NqnStrLen;
    PUCHAR Disk;                    //Memory of RamDisk.

    UINT64 TotalDiskBytes;
    ULONG BlockSizeInBytes;         //Size of LBA block, in bytes.
    ULONG_PTR TotalBlocks;
    
    //Max LBA index of this disk. 
    //e.g. when TotalBlocks==10, MaxLBA will be 9 because LBA is zero-based index.
    INT64 MaxLBA;                   
    BOOLEAN ReadCacheEnabled;
    BOOLEAN WriteCacheEnabled;

    void Setup();
    void Teardown();

    BOOLEAN IsValidOffsetAndLength(ULONG_PTR offset, ULONG length);
    BOOLEAN IsValidLbaAndLength(ULONG_PTR lba_start, ULONG block_count);
    NTSTATUS ReadLBA(ULONG_PTR start_block, ULONG blocks, PVOID buffer);
    NTSTATUS WriteLBA(ULONG_PTR start_block, ULONG blocks, PVOID buffer);
    NTSTATUS Read(ULONG_PTR offset, ULONG length, PVOID buffer);
    NTSTATUS Write(ULONG_PTR offset, ULONG length, PVOID buffer);

#if 0
    void PushIoRequest(PVOID srbext);
    PVOID PopIoRequest();
    ULONG ProcessIoRequests();
    ULONG AbortIoRequests();
    void StartWorkerThread();
    void StopWorkerThread();
#endif
    void SetDiskSize(size_t total_size, ULONG size_of_block);
    void LoadRegistry();
    void LoadDefault();
}SPC_DEVEXT, * PSPC_DEVEXT;
