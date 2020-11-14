.model flat

;windows 2000 sp4
handledGetLogicalProcessorInformation PROTO STDCALL :DWORD,:DWORD
handledInterlockedFlushSList PROTO STDCALL :DWORD
handledInterlockedPopEntrySList PROTO STDCALL :DWORD
handledInitializeSListHead PROTO STDCALL :DWORD
handledInterlockedPushEntrySList  PROTO STDCALL :DWORD,:DWORD
handledQueryDepthSList  PROTO STDCALL :DWORD
handledGetNumaHighestNodeNumber  PROTO STDCALL :DWORD
handledConvertFiberToThread  PROTO STDCALL
handledGetModuleHandleExW PROTO STDCALL :DWORD,:DWORD,:DWORD
handledGetModuleHandleExA PROTO STDCALL :DWORD,:DWORD,:DWORD
handledGetVersionExW  PROTO STDCALL :DWORD

;windows xp no service pack
handledEncodePointer PROTO STDCALL :DWORD
handledDecodePointer PROTO STDCALL :DWORD

.data
__imp__EncodePointer@4 dd handledEncodePointer
__imp__DecodePointer@4 dd handledDecodePointer
__imp__ConvertFiberToThread@0 dd handledConvertFiberToThread
__imp__InitializeSListHead@4 dd handledInitializeSListHead
__imp__InterlockedFlushSList@4 dd handledInterlockedFlushSList
__imp__InterlockedPopEntrySList@4 dd handledInterlockedPopEntrySList
__imp__InterlockedPushEntrySList@8 dd handledInterlockedPushEntrySList 
__imp__QueryDepthSList@4 dd handledQueryDepthSList
__imp__GetModuleHandleExW@12 dd handledGetModuleHandleExW
__imp__GetModuleHandleExA@12 dd handledGetModuleHandleExA
__imp__GetNumaHighestNodeNumber@4 dd handledGetNumaHighestNodeNumber
__imp__GetLogicalProcessorInformation@8 dd handledGetLogicalProcessorInformation
__imp__GetVersionExW@4 dd handledGetVersionExW

EXTERNDEF __imp__EncodePointer@4 : DWORD
EXTERNDEF __imp__DecodePointer@4 : DWORD
EXTERNDEF __imp__ConvertFiberToThread@0 : DWORD
EXTERNDEF __imp__InitializeSListHead@4 : DWORD
EXTERNDEF __imp__InterlockedFlushSList@4 : DWORD
EXTERNDEF __imp__InterlockedPopEntrySList@4 : DWORD
EXTERNDEF __imp__InterlockedPushEntrySList@8 : DWORD
EXTERNDEF __imp__GetModuleHandleExW@12 : DWORD
EXTERNDEF __imp__GetModuleHandleExA@12 : DWORD
EXTERNDEF __imp__GetNumaHighestNodeNumber@4 : DWORD
EXTERNDEF __imp__QueryDepthSList@4 : DWORD
EXTERNDEF __imp__GetLogicalProcessorInformation@8 : DWORD
EXTERNDEF __imp__GetVersionExW@4 : DWORD

end