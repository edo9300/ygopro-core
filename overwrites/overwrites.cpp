#ifndef _WIN64
#include <Windows.h>
#include <winternl.h>

//some implementations taken from https://sourceforge.net/projects/win2kxp/

/*
//can't use c runtime functions as the runtime might not have been loaded yet
void ___write(const char* ch) {
	DWORD dwCount, dwMsgLen;
	static HANDLE hOut;
	if(!hOut)
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hOut, ch, strlen(ch), &dwCount, NULL);
}*/

/*
creates 2 functions, the stub function prefixed by handledxxx that is then exported via asm,
and internalimplxxx that is the fallback function called if the function isn't loaded at runtime
on first call GetProcAddress is called, and if the function is found, then that will be called onwards
otherwise fall back to the internal implementation
*/
#define GETFUNC(funcname) (decltype(&handled##funcname))GetProcAddress(GetModuleHandle(LIBNAME), #funcname)
#define MAKELOADER(funcname,ret,args,argnames) \
ret __stdcall internalimpl##funcname args ; \
extern "C" ret __stdcall handled##funcname args; \
const auto basefunc##funcname = [] { \
	auto func = GETFUNC(funcname); \
	return func ? func : internalimpl##funcname; \
}(); \
extern "C" ret __stdcall handled##funcname args { \
	return basefunc##funcname argnames ; \
} \
ret __stdcall internalimpl##funcname args

#define MAKELOADER_WITH_CHECK(funcname,ret,args,argnames) \
ret __stdcall internalimpl##funcname args ; \
extern "C" ret __stdcall handled##funcname args; \
const auto basefunc##funcname = [] { \
	auto func = GETFUNC(funcname); \
	return func ? func : internalimpl##funcname; \
}(); \
extern "C" ret __stdcall handled##funcname args { \
	if(!basefunc##funcname) { \
		auto func = GETFUNC(funcname); \
		return (func ? func : internalimpl##funcname) argnames ; \
	} \
	return basefunc##funcname argnames; \
} \
ret __stdcall internalimpl##funcname args

#define LIBNAME TEXT("kernel32.dll")

typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _PEB_LDR_DATA_2K {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_2K, *PPEB_LDR_DATA_2K;

typedef struct _RTL_USER_PROCESS_PARAMETERS_2K {
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE StdInputHandle;
	HANDLE StdOutputHandle;
	HANDLE StdErrorHandle;
} RTL_USER_PROCESS_PARAMETERS_2K, *PRTL_USER_PROCESS_PARAMETERS_2K;
typedef struct _PEB_2K {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN Spare;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA_2K LoaderData;
	PRTL_USER_PROCESS_PARAMETERS_2K ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	//And more but we don't care...
	DWORD reserved[0x21];
	PRTL_CRITICAL_SECTION LoaderLock;
} PEB_2K, *PPEB_2K;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

typedef struct _TEB_2K {
	NT_TIB NtTib;
	PVOID EnvironmentPointer;
	CLIENT_ID Cid;
	PVOID ActiveRpcInfo;
	PVOID ThreadLocalStoragePointer;
	PPEB_2K Peb;
	DWORD reserved[0x3D0];
	BOOLEAN InDbgPrint; // 0xF74
	BOOLEAN FreeStackOnTermination;
	BOOLEAN HasFiberData;
	UCHAR IdealProcessor;
} TEB_2K, *PTEB_2K;
inline PTEB_2K NtCurrentTeb2k(void) {
	return (PTEB_2K)NtCurrentTeb();
}

inline PPEB_2K NtCurrentPeb(void) {
	return NtCurrentTeb2k()->Peb;
}
static PLDR_MODULE __stdcall GetLdrModule(LPCVOID address) {
	PLDR_MODULE first_mod, mod;
	first_mod = mod = (PLDR_MODULE)NtCurrentPeb()->LoaderData->InLoadOrderModuleList.Flink;
	do {
		if((ULONG_PTR)mod->BaseAddress <= (ULONG_PTR)address &&
			(ULONG_PTR)address < (ULONG_PTR)mod->BaseAddress + mod->SizeOfImage)
			return mod;
		mod = (PLDR_MODULE)mod->InLoadOrderModuleList.Flink;
	} while(mod != first_mod);
	return nullptr;
}

static VOID LoaderLock(BOOL lock) {
	if(lock)
		EnterCriticalSection(NtCurrentPeb()->LoaderLock);
	else
		LeaveCriticalSection(NtCurrentPeb()->LoaderLock);
}
static HMODULE GetModuleHandleFromPtr(LPCVOID p) {
	PLDR_MODULE pLM;
	HMODULE ret;
	LoaderLock(TRUE);
	pLM = GetLdrModule(p);
	if(pLM)
		ret = (HMODULE)pLM->BaseAddress;
	else
		ret = nullptr;
	LoaderLock(FALSE);
	return ret;
}

BYTE slist_lock[0x100];
void SListLock(PSLIST_HEADER ListHead) {
	DWORD index = (((DWORD)ListHead) >> MEMORY_ALLOCATION_ALIGNMENT) & 0xFF;

	__asm {
		mov edx, index
		mov cl, 1
		spin:	mov al, 0
				lock cmpxchg byte ptr[slist_lock + edx], cl
				jnz spin
	};
	return;
}

void SListUnlock(PSLIST_HEADER ListHead) {
	DWORD index = (((DWORD)ListHead) >> MEMORY_ALLOCATION_ALIGNMENT) & 0xFF;
	slist_lock[index] = 0;
}

//Note: ListHead->Next.N== first node

MAKELOADER(InterlockedFlushSList, PSLIST_ENTRY, (PSLIST_HEADER ListHead), (ListHead)) {
	PSLIST_ENTRY ret;
	SListLock(ListHead);
	ret = ListHead->Next.Next;
	ListHead->Next.Next = nullptr;
	ListHead->Depth = 0;
	ListHead->Sequence++;
	SListUnlock(ListHead);
	return ret;
}

MAKELOADER_WITH_CHECK(InitializeSListHead, void, (PSLIST_HEADER ListHead), (ListHead)) {
	SListLock(ListHead);
	ListHead->Next.Next = nullptr;
	ListHead->Depth = 0;
	ListHead->Sequence = 0;
	SListUnlock(ListHead);
}

static BOOL IncLoadCount(HMODULE hMod) {
	WCHAR path[MAX_PATH];
	DWORD c = GetModuleFileNameW(hMod, path, MAX_PATH);
	if(c != MAX_PATH) {
		return LoadLibraryW(path) != nullptr;
	}
	return FALSE;
}

MAKELOADER(GetModuleHandleExW, BOOL, (DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule), (dwFlags, lpModuleName, phModule)) {
	LoaderLock(TRUE);
	if(dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
		*phModule = GetModuleHandleFromPtr(lpModuleName);
	else
		*phModule = GetModuleHandleW(lpModuleName);

	if(*phModule == nullptr) {
		LoaderLock(FALSE);
		return FALSE;
	}
	if(!(dwFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT) ||
	   dwFlags & GET_MODULE_HANDLE_EX_FLAG_PIN) {
		//How to pin? We'll just inc the LoadCount and hope
		IncLoadCount(*phModule);
	}

	LoaderLock(FALSE);
	return TRUE;
}

MAKELOADER(EncodePointer, PVOID, (PVOID ptr), (ptr)) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

MAKELOADER(DecodePointer, PVOID, (PVOID ptr), (ptr)) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}
#endif
