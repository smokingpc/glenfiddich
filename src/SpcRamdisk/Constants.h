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


#define DBG_FILTER          0x00000080
#define KiloBytes           (1<<10)
#define MegaBytes           (1<<20)     //1024*1024
#define GigaBytes           (1<<30)     //1024*1024*1024
#define MAX_TX_PAGES        (256)
#define MAX_TX_SIZE         (MAX_TX_PAGES * PAGE_SIZE)
#define MAX_IO_PER_LUN      1024
#define SUPPORTED_LU        1
#define MAX_TOTAL_IO        (MAX_IO_PER_LUN*SUPPORTED_LU)
#define STALL_TIMEOUT       500     //in micro-second
#define MIN_CONCURRENT_IO   8
#define PAUSE_ADAPTER_TIMEOUT   5//in seconds

#define INVALID_RAIDPORT    (MAXUSHORT)
#define INVALID_BUS_ID      (MAXUCHAR)
#define INVALID_TARGET_ID   (MAXUCHAR)
#define INVALID_LUN_ID      (MAXUCHAR)
#define INVALID_SCSI_TAG    (MAXULONG)

#define DEFAULT_BLOCK_SIZE    4096
#define DEFAULT_BLOCK_SIZE2   512
#define DEFAULT_DISK_BYTES    128 * MegaBytes
#define WORKER_PROCESS_COUNT  128  //how many requests worker thread handled in each loop?
#define WORKER_INTERVAL       (-10*1000)    //how long worker thread wait in each loop?
#define SN_STRBUF_SIZE      32
#define VENDOR_ID           "SPC"  //MAX 8 chars
#define PRODUCT_ID          "SpcRamDisk      "  //MAX 16 chars
#define REV_ID              "0100"      //MAX 4
#define NQN_BUF_SIZE        224
#define NQN_FORMAT          "nqn.2014-08.smokingpc:spc.ramdisk_%s"
#define DISK_SN             (LONGLONG)1645017030  //epoc timestamp (2022/02/16 21:10:30)

#define OVERRUN_TAG         (0x23939889)
