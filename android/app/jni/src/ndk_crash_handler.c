// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Jaime Ita Passos.
// Copyright (C) 2025 by StarManiaKG & Bitten2Up.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ndk_crash_handler.c
/// \brief Android crash handler

#include "ndk_crash_handler.h"
#include "jni_android.h"

#include <stdio.h>
#include <unwind.h>
#include <dlfcn.h>

typedef struct BacktraceState
{
	void **current;
	void **end;
} BacktraceState_t;

static _Unwind_Reason_Code NDKCrashHandler_Unwind(struct _Unwind_Context* context, void* arg)
{
	BacktraceState_t *state = (BacktraceState_t *)(arg);
	uintptr_t pc = _Unwind_GetIP(context);
	if (pc)
	{
		if (state->current == state->end)
			return _URC_END_OF_STACK;
		else
			*state->current++ = (void *)(pc);
	}
	return _URC_NO_REASON;
}

size_t NDKCrashHandler_CaptureBacktrace(void **buffer, size_t max)
{
	BacktraceState_t state = {buffer, buffer + max};
	_Unwind_Backtrace(NDKCrashHandler_Unwind, &state);
	return state.current - buffer;
}

static void NDKCrashHandler_PrintToLog(FILE *log_file, const char *fmt, ...)
{
	static char txt[8192] = "";

	if (!log_file)
		return;

#if 0
	// STAR NOTE: normal
	va_list argptr;
	va_start(argptr, fmt);
	Android_vsnprintf(txt, 8192, fmt, argptr);
	va_end(argptr);
#else
	strlcat(txt, fmt, 8192);
#endif

	fwrite(txt, strlen(txt), 1, log_file);
	CON_LogMessage(txt);
}

static void NDKCrashHandler_StackTrace(FILE *stacktrace_file)
{
	const int max = 4096;
	void *buffer[max];
	size_t count = NDKCrashHandler_CaptureBacktrace(buffer, max);
	int i;

	if (!count)
		return;

	NDKCrashHandler_PrintToLog(stacktrace_file, "Stack trace:\n");
	for (i = 0; i < count; i++)
	{
		Dl_info info;
		const void *addr = buffer[i];
		const char *symbol = "(mangled)";

		if (dladdr(addr, &info) && info.dli_sname)
			symbol = info.dli_sname;

		NDKCrashHandler_PrintToLog(stacktrace_file, "%d: %p %s\n", i, addr, symbol);
	}
}

void NDKCrashHandler_ReportSignal(const char *sigmsg, int signum)
{
	static FILE *crash_log = NULL;
	INT32 i;

	// open crash-log.txt
	crash_log = fopen(va("%s/crash-log.txt", I_SharedStorageLocation()), "wt+");

#if 0
	// Get the current time as a string.
	time_t rawtime;
	struct tm timeinfo;
	char timestr[32];
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	strftime(timestr, 32, "%a, %d %b %Y %T %z", &timeinfo);

	// Let the user know what the heck is happening
	int fd = -1;
	bt_write_file(fd, "------------------------\n"); // Nice looking seperator
	bt_write_all(fd, "An error occurred within SRB2! Send this stack trace to someone who can help!\n");
	if (fd != -1) // If the crash log exists,
		bt_write_stderr("(Or find crash-log.txt in your SRB2 directory.)\n"); // tell the user where the crash log is.

	// Tell the log when we crashed.
	bt_write_file(fd, "Time of crash: ");
	bt_write_file(fd, timestr);
	bt_write_file(fd, "\n");

	// Give the crash log the cause and a nice 'Backtrace:' thing
	// The signal is given to the user when the parent process sees we crashed.
	bt_write_file(fd, "Cause: ");
	bt_write_file(fd, strsignal(signum));
	bt_write_file(fd, "\n"); // Newline for the signal name
#endif

	// write crash info
	NDKCrashHandler_PrintToLog(crash_log, "Application killed by signal: %d, %s\n\n", signum, sigmsg);
	NDKCrashHandler_PrintToLog(crash_log, "Device info:\n", sigmsg);
	for (i = 0; JNI_DeviceInfoReference[i].info; i++)
	{
		JNI_DeviceInfoReference_t *ref = &JNI_DeviceInfoReference[i];
		JNI_DeviceInfo_t info_e = ref->info_enum;
		NDKCrashHandler_PrintToLog(crash_log, "\t%s: %s\n", ref->display_info, JNI_DeviceInfo[info_e]);
	}

	if (JNI_ABICount)
	{
		NDKCrashHandler_PrintToLog(crash_log, "Supported ABIs: ");
		for (i = 0; i < JNI_ABICount; i++)
		{
			NDKCrashHandler_PrintToLog(crash_log, "%s", JNI_ABIList[i]);
			if (i == JNI_ABICount-1)
				NDKCrashHandler_PrintToLog(crash_log, "\n");
			else
				NDKCrashHandler_PrintToLog(crash_log, ", ");
		}
	}

	NDKCrashHandler_PrintToLog(crash_log, "\n");
	NDKCrashHandler_StackTrace(crash_log);

	if (crash_log)
	{
		fclose(crash_log);
		crash_log = NULL;
	}
}
