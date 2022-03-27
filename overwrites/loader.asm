IFDEF RAX
    END_IF_NOT_X86 equ end  ; kill stop the assembler if we're not compiling for x86
ELSE
    END_IF_NOT_X86 equ <>
ENDIF

END_IF_NOT_X86

.model flat
.data

;windows 2000 sp4
handledInterlockedFlushSList PROTO STDCALL :DWORD
__imp__InterlockedFlushSList@4 dd handledInterlockedFlushSList
public __imp__InterlockedFlushSList@4

handledInitializeSListHead PROTO STDCALL :DWORD
__imp__InitializeSListHead@4 dd handledInitializeSListHead
public __imp__InitializeSListHead@4

handledGetModuleHandleExW PROTO STDCALL :DWORD,:DWORD,:DWORD
__imp__GetModuleHandleExW@12 dd handledGetModuleHandleExW
public __imp__GetModuleHandleExW@12

;windows xp no service pack
handledEncodePointer PROTO STDCALL :DWORD
__imp__EncodePointer@4 dd handledEncodePointer
public __imp__EncodePointer@4

handledDecodePointer PROTO STDCALL :DWORD
__imp__DecodePointer@4 dd handledDecodePointer
public __imp__DecodePointer@4

end