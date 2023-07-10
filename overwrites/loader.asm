IFDEF RAX
    END_IF_NOT_X86 equ end  ; kill stop the assembler if we're not compiling for x86
ELSE
    END_IF_NOT_X86 equ <>
ENDIF

END_IF_NOT_X86

.model flat
LoadSymbols PROTO STDCALL

;Replace every overridden __imp__ symbol with a trampoline code
;that will call the function LoadSymbols, after that function is called
;all the overridden __imp__ symbols will be pointing to the right function
;the macro also exports a symbol called "functionname"symbol that will hold
;a pointer to the __imp__ symbol, so that it can be referenced from c/c++ code
;as symbols with @ in their name can't be referenced from there

DECLARE_STUB MACRO functionname, parameters
	LOCAL L1
.code
	L1:
		CALL LoadSymbols
		JMP __imp__&functionname&@&parameters&
.data
	__imp__&functionname&@&parameters& DD L1
	public __imp__&functionname&@&parameters&
	&functionname&symbol DD __imp__&functionname&@&parameters&
	public c &functionname&symbol
ENDM

;windows xp no service pack
DECLARE_STUB EncodePointer, 4
DECLARE_STUB DecodePointer, 4

end
