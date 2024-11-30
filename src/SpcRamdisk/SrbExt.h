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

//SRB Extension for Ramdisk, this is "per STORPORT REQUEST" context
typedef struct _SPC_SRBEXT
{
    SLIST_ENTRY List;
    PSPC_DEVEXT DevExt;
    PSCSI_REQUEST_BLOCK  Srb;
    UCHAR SrbStatus;

    ULONG FuncCode;
    ULONG ScsiTag;
    UCHAR CdbLen;
    PCDB Cdb;
    USHORT RaidPort;//Storport RAID Port of this adapter. You can see it in windbg !storadapter detail.
    UCHAR Bus;      //"Bus" of Storport BTL8 address
    UCHAR Target;   //"Target" of Storport BTL8 address
    UCHAR Lun;      //"Lun" of Storport BTL8 address

    ULONG64 RwOffset;       //read/write offset in LBA blocks, not bytes.(0 if it's not read/write request)
    ULONG RwLength;         //read/write in LBA blocks, not bytes.(0 if it's not read/write request)
    ULONG64 RwOffsetBytes;  //read/write offset in bytes.(0 if it's not read/write request)
    ULONG RwLengthBytes;    //read/write in bytes.(0 if it's not read/write request)

    PVOID DataBuf;
    ULONG DataBufLen;
    BOOLEAN IsScsiWrite;
    ULONG Protect;
}SPC_SRBEXT, * PSPC_SRBEXT;

PSPC_SRBEXT GetSrbExt(_In_ PSCSI_REQUEST_BLOCK srb, _In_ PVOID devext);
void CompleteSrb(_In_ PSPC_SRBEXT srbext, _In_ UCHAR srb_status);

//If the request need reply data or required buffer length,
//storport driver should update DataBuffer Length by SrbSetDataTransferLength().
//Storport will copy back data by this length, like BUFERRED_IO of IOCTL.
void UpdateDataBufLen(_In_ PSPC_SRBEXT srbext, _In_ ULONG len);

bool GetSrbPnpRequest(
    _In_ PSPC_SRBEXT srbext, 
    _Inout_ ULONG& flags, 
    _Inout_ STOR_PNP_ACTION& action);
