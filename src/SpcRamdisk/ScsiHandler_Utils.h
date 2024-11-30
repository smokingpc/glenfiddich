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

void CopyToCdbBuffer(
    PUCHAR& buffer, 
    ULONG& buf_size, 
    PVOID page, 
    ULONG& copy_size, 
    ULONG& ret_size);
void FillParamHeader(PMODE_PARAMETER_HEADER header);
void FillParamHeader10(PMODE_PARAMETER_HEADER10 header);
void FillModePage_Caching(PMODE_CACHING_PAGE page);
void FillModePage_InfoException(PMODE_INFO_EXCEPTIONS page);
void FillModePage_Control(PMODE_CONTROL_PAGE page);
void ReplyModePageCaching(
    PSPC_DEVEXT devext,
    PUCHAR& buffer, 
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size);
void ReplyModePageControl(
    PSPC_DEVEXT devext,
    PUCHAR& buffer,
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size);
void ReplyModePageInfoExceptionCtrl(
    PSPC_DEVEXT devext,
    PUCHAR& buffer,
    ULONG& buf_size, 
    ULONG& mode_page_size, 
    ULONG& ret_size);
UCHAR ReadWriteRamdisk(PSPC_SRBEXT srbext, BOOLEAN is_write);
