#include "StdAfx.h"
#include "error.h"

#if defined(_WIN32)

#include <stdio.h>
#include <time.h>
#include <winsock.h>
#include <imagehlp.h>

FILE * fException;

BOOL CALLBACK EnumerateLoadedModulesProc(PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	DWORD offset = *((DWORD*)UserContext);
	
	if (offset >= ModuleBase && offset <= ModuleBase + ModuleSize)
	{
		fprintf(fException, "%s", ModuleName);
		return FALSE;
	}
	else
		return TRUE;
}

LONG __stdcall EterExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	HANDLE		hProcess	= GetCurrentProcess();
	HANDLE		hThread		= GetCurrentThread();
	
	fException = fopen("ErrorLog.txt", "wt");
	if (fException)
	{
		char module_name[256];
		time_t module_time;
		
		HMODULE hModule = GetModuleHandle(NULL);
		
		GetModuleFileName(hModule, module_name, sizeof(module_name));
		module_time = (time_t)GetTimestampForLoadedLibrary(hModule);
		
		fprintf(fException, "Module Name: %s\n", module_name);
		fprintf(fException, "Time Stamp: %lld - %s\n", (long long)module_time, ctime(&module_time));
		fprintf(fException, "\n");
		fprintf(fException, "Exception Type: 0x%08x\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
		fprintf(fException, "\n");
		
		CONTEXT&	context		= *pExceptionInfo->ContextRecord;
		
		fprintf(fException, "eax: 0x%08x\tebx: 0x%08x\n", context.Eax, context.Ebx);
		fprintf(fException, "ecx: 0x%08x\tedx: 0x%08x\n", context.Ecx, context.Edx);
		fprintf(fException, "esi: 0x%08x\tedi: 0x%08x\n", context.Esi, context.Edi);
		fprintf(fException, "ebp: 0x%08x\tesp: 0x%08x\n", context.Ebp, context.Esp);
		fprintf(fException, "\n");
		
		STACKFRAME stackFrame = {0,};
		stackFrame.AddrPC.Offset	= context.Eip;
		stackFrame.AddrPC.Mode		= AddrModeFlat;
		stackFrame.AddrStack.Offset	= context.Esp;
		stackFrame.AddrStack.Mode	= AddrModeFlat;
		stackFrame.AddrFrame.Offset	= context.Ebp;
		stackFrame.AddrFrame.Mode	= AddrModeFlat;
		
		for (int i=0; i < 512 && stackFrame.AddrPC.Offset; ++i)
		{
			if (StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame, &context, NULL, NULL, NULL, NULL) != FALSE)
			{
				fprintf(fException, "0x%08x\t", stackFrame.AddrPC.Offset);
				EnumerateLoadedModules(hProcess, (PENUMLOADED_MODULES_CALLBACK) EnumerateLoadedModulesProc, &stackFrame.AddrPC.Offset);
				fprintf(fException, "\n");
			}
			else
			{
				break;
			}
		} 
		
		fprintf(fException, "\n");

		fflush(fException);

		fclose(fException);
		fException = NULL;
		
		MessageBox(NULL, "A fatal error was encountered while running the game.\nAn error log was saved to ErrorLog.txt.\nPlease consider reporting this issue.", "Error!", MB_OK);
	}
	
	return EXCEPTION_EXECUTE_HANDLER;
}

void SetEterExceptionHandler()
{
	SetUnhandledExceptionFilter(EterExceptionFilter);
}

#else

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <execinfo.h>

#include <ucontext.h>

static void PosixSignalHandler(int sig, siginfo_t* info, void* ucontext)
{
	FILE* f = fopen("ErrorLog.txt", "wt");
	if (f)
	{
		fprintf(f, "Fatal Signal: %d, faulting address: %p\n", sig, info ? info->si_addr : nullptr);
		if (ucontext)
		{
			ucontext_t* uc = (ucontext_t*)ucontext;
#if defined(__x86_64__)
			fprintf(f, "RIP: 0x%llx, RSP: 0x%llx, RBP: 0x%llx\n",
				(unsigned long long)uc->uc_mcontext.gregs[REG_RIP],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RSP],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RBP]);
			fprintf(f, "RAX: 0x%llx, RBX: 0x%llx, RCX: 0x%llx, RDX: 0x%llx\n",
				(unsigned long long)uc->uc_mcontext.gregs[REG_RAX],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RBX],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RCX],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RDX]);
			fprintf(f, "RSI: 0x%llx, RDI: 0x%llx\n",
				(unsigned long long)uc->uc_mcontext.gregs[REG_RSI],
				(unsigned long long)uc->uc_mcontext.gregs[REG_RDI]);

			// Dump top 24 words on stack
			uintptr_t* sp = (uintptr_t*)uc->uc_mcontext.gregs[REG_RSP];
			if (sp)
			{
				fprintf(f, "Stack dump:\n");
				for (int i = 0; i < 24; ++i)
				{
					fprintf(f, "  [%02d] %p: 0x%016lx\n", i, (void*)&sp[i], (unsigned long)sp[i]);
				}
			}
#endif
		}
		void* buffer[64];
		int nptrs = backtrace(buffer, 64);
		char** strings = backtrace_symbols(buffer, nptrs);
		if (strings)
		{
			fprintf(f, "Backtrace:\n");
			for (int i = 0; i < nptrs; ++i)
				fprintf(f, "%s\n", strings[i]);
			free(strings);
		}
		fclose(f);
	}
	_exit(1);
}

void SetEterExceptionHandler()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = PosixSignalHandler;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
}

#endif
