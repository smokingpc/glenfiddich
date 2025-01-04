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


#pragma once
#include <wdm.h>
EXTERN_C_START
#include <storport.h>
#include <srbhelper.h>
EXTERN_C_END


#include <ntstrsafe.h>
#include "PoolTag.h"
#include "Constants.h"
#include "Enumeration.h"
#include "AutoPointer.hpp"
#include "DebugUtils.h"
#include "CppSupport.h"

#include "DevExt.h"
#include "SrbExt.h"
#include "EventTracingLog.h"
#include "MiniportFunction.h"
#include "AdapterControl_Handlers.h"
#include "UnitControl_Handlers.h"
#include "StartIo_Handler.h"

#include "ScsiHandler_CDB6.h"
#include "ScsiHandler_CDB10.h"
#include "ScsiHandler_CDB12.h"
#include "ScsiHandler_CDB16.h"
#include "ScsiHandler_Utils.h"

