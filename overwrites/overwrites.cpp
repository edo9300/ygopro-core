#ifndef _WIN64
#include <Windows.h>
#include <array>
#include <utility>

namespace {
namespace Detail {

template<size_t N>
struct Callable;

#define STUB_WITH_LIBRARY_INT(index,funcname,ret,args,libraryName) \
extern "C" FARPROC* funcname##symbol; \
ret __stdcall internalimpl##funcname args ; \
template<> \
struct Detail::Callable<index> { \
	static constexpr auto& reference_symbol{funcname##symbol}; \
	static auto __stdcall load() { \
		const auto loaded = reinterpret_cast<FARPROC>(GetProcAddress(GetModuleHandle(libraryName), #funcname)); \
		return loaded ? loaded : reinterpret_cast<FARPROC>(internalimpl##funcname); \
	} \
}; \
ret __stdcall internalimpl##funcname args

#define STUB_WITH_LIBRARY(funcname,ret,args) STUB_WITH_LIBRARY_INT(__COUNTER__, funcname,ret,args, LIBNAME)

struct Args {
	FARPROC*& targetSymbol;
	FARPROC(__stdcall* load)();
};

template<size_t... I>
constexpr auto make_overridden_functions_array(std::index_sequence<I...> seq) {
	return std::array<Args, seq.size()>{Args{ Detail::Callable<I>::reference_symbol, Detail::Callable<I>::load }...};
}

} // namespace Detail
} // namespace

#define GET_OVERRIDDEN_FUNCTIONS_ARRAY() \
	Detail::make_overridden_functions_array(std::make_index_sequence<__COUNTER__>());

namespace {

#define LIBNAME TEXT("kernel32.dll")

STUB_WITH_LIBRARY(EncodePointer, PVOID, (PVOID ptr)) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

STUB_WITH_LIBRARY(DecodePointer, PVOID, (PVOID ptr)) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

/*
* Manually replace the various __imp__ pointers with the right function
* (or the one loaded from a dll or the reimplementation)
* this relies on a symbol holding the function pointer to be exported
* from the asm file that will then be referenced in the functions array
*/
extern "C" void __stdcall LoadSymbols() {
	static constexpr auto functionArray = GET_OVERRIDDEN_FUNCTIONS_ARRAY();
	for(const auto& obj : functionArray)
		*obj.targetSymbol = obj.load();
}
}
#endif
