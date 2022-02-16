.model flat

;windows 2000 sp4
handledInterlockedFlushSList PROTO STDCALL :DWORD
handledInitializeSListHead PROTO STDCALL :DWORD
handledGetModuleHandleExW PROTO STDCALL :DWORD,:DWORD,:DWORD

;windows xp no service pack
handledEncodePointer PROTO STDCALL :DWORD
handledDecodePointer PROTO STDCALL :DWORD

.data
__imp__EncodePointer@4 dd handledEncodePointer
__imp__DecodePointer@4 dd handledDecodePointer
__imp__GetModuleHandleExW@12 dd handledGetModuleHandleExW
__imp__InitializeSListHead@4 dd handledInitializeSListHead
__imp__InterlockedFlushSList@4 dd handledInterlockedFlushSList

EXTERNDEF __imp__EncodePointer@4 : DWORD
EXTERNDEF __imp__DecodePointer@4 : DWORD
EXTERNDEF __imp__InitializeSListHead@4 : DWORD
EXTERNDEF __imp__InterlockedFlushSList@4 : DWORD
EXTERNDEF __imp__GetModuleHandleExW@12 : DWORD

end