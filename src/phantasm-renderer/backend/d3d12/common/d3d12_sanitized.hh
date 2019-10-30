#pragma once
#ifdef PR_BACKEND_D3D12

// Check if CC win32 sanitized was included before this error
#ifdef CC_SANITIZED_WINDOWS_H
#error "This header is incompatbile with clean-core's sanitized win32"
#endif

// Check if <Windows.h> was included somewhere before this header
#if defined(_WINDOWS_) && !defined(PR_SANITIZED_D3D12_H)
//#error "Including unsanitized d3d12.h or Windows.h"
#endif
#define PR_SANITIZED_D3D12_H

// Exclude MFC features
//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif
//#ifndef VC_EXTRALEAN
//#define VC_EXTRALEAN
//#endif

// Use STRICT
// https://docs.microsoft.com/en-us/windows/win32/winprog/enabling-strict
//#ifndef STRICT
//#define STRICT
//#endif

// NO<Feature> macros, list from Microsoft AirSim
// https://github.com/microsoft/AirSim/blob/master/AirLib/include/common/common_utils/MinWinDefines.hpp

/*
    The MIT License (MIT)

    MSR Aerial Informatics and Robotics Platform
    MSR Aerial Informatics and Robotics Simulator (AirSim)

    Copyright (c) Microsoft Corporation

    All rights reserved.

    MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//#define NOGDICAPMASKS // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
////#define NOVIRTUALKEYCODES		// VK_*
////#define NOWINMESSAGES			// WM_*, EM_*, LB_*, CB_*
////#define NOWINSTYLES			// WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
////#define NOSYSMETRICS			// SM_*
//// #define NOMENUS				// MF_*
////#define NOICONS				// IDI_*
////#define NOKEYSTATES			// MK_*
////#define NOSYSCOMMANDS			// SC_*
////#define NORASTEROPS			// Binary and Tertiary raster ops
////#define NOSHOWWINDOW			// SW_*
//#define OEMRESOURCE // OEM Resource values
//#define NOATOM      // Atom Manager routines
////#define NOCLIPBOARD			// Clipboard routines
////#define NOCOLOR				// Screen colors
////#define NOCTLMGR				// Control and Dialog routines
//#define NODRAWTEXT // DrawText() and DT_*
////#define NOGDI					// All GDI #defines and routines
//#define NOKERNEL // All KERNEL #defines and routines
////#define NOUSER				// All USER #defines and routines
////#define NONLS					// All NLS #defines and routines
////#define NOMB					// MB_* and MessageBox()
//#define NOMEMMGR   // GMEM_*, LMEM_*, GHND, LHND, associated routines
//#define NOMETAFILE // typedef METAFILEPICT
//#define NOMINMAX   // Macros min(a,b) and max(a,b)
////#define NOMSG					// typedef MSG and associated routines
//#define NOOPENFILE // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
//#define NOSCROLL   // SB_* and scrolling routines
//#define NOSERVICE  // All Service Controller routines, SERVICE_ equates, etc.
//#define NOSOUND    // Sound driver routines
////#define NOTEXTMETRIC			// typedef TEXTMETRIC and associated routines
////#define NOWH					// SetWindowsHook and WH_*
////#define NOWINOFFSETS			// GWL_*, GCL_*, associated routines
//#define NOCOMM           // COMM driver routines
//#define NOKANJI          // Kanji support stuff.
//#define NOHELP           // Help engine interface.
//#define NOPROFILER       // Profiler interface.
//#define NODEFERWINDOWPOS // DeferWindowPos routines
//#define NOMCX            // Modem Configuration Extensions
//#define NOCRYPT
//#define NOTAPE
//#define NOIMAGE
//#define NOPROXYSTUB
//#define NORPC

struct IUnknown;

#pragma warning(push, 0)
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <combaseapi.h>
#pragma warning(pop)

//#undef near
//#undef far

//#undef INT
//#undef UINT
//#undef DWORD
//#undef FLOAT
//#undef uint8
//#undef uint16
//#undef uint32
//#undef int32
//#undef float

//#undef PF_MAX
//#undef PlaySound
//#undef DrawText
//#undef CaptureStackBackTrace
//#undef MemoryBarrier
//#undef DeleteFile
//#undef MoveFile
//#undef CopyFile
//#undef CreateDirectory
//#undef GetCurrentTime
//#undef SendMessage
//#undef LoadString
//#undef UpdateResource
//#undef FindWindow
//#undef GetObject
//#undef GetEnvironmentVariable
//#undef CreateFont
//#undef CreateDesktop
//#undef GetMessage
//#undef GetCommandLine
//#undef GetProp
//#undef SetProp
//#undef GetFileAttributes
//#undef ReportEvent
//#undef GetClassName
//#undef GetClassInfo
//#undef Yield
//#undef IMediaEventSink

#endif
