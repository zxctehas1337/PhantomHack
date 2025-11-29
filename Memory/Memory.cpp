#include "../Precompiled.h"

bool Memory::Setup()
{
	bool bSuccess = true;

	void* ntHandle = GetModuleBaseHandle(X(L"ntdll.dll"));
	NtProtectVirtualMemory = (tNtProtectVirtualMemory)GetExportAddress(ntHandle, X("NtProtectVirtualMemory"));
	NtQueryVirtualMemory = (tNtQueryVirtualMemory)GetExportAddress(ntHandle, X("NtQueryVirtualMemory"));
	bSuccess &= (NtProtectVirtualMemory != nullptr && NtQueryVirtualMemory != nullptr);

	const void* hDbgHelp = GetModuleBaseHandle(DBGHELP_DLL);
	const void* hTier0 = GetModuleBaseHandle(TIER0_DLL);
	if (!hDbgHelp || !hTier0)
		return false;

	while (!GetModuleBaseHandle(X(L"navsystem.dll")))
	{
	};
	fnUnDecorateSymbolName = reinterpret_cast<decltype(fnUnDecorateSymbolName)>(GetExportAddress(hDbgHelp, X("UnDecorateSymbolName")));
	bSuccess &= (fnUnDecorateSymbolName != nullptr);

	Functions::fnUtlBufferInit = reinterpret_cast<decltype(Functions::fnUtlBufferInit)>(GetExportAddress(hTier0, X("??0CUtlBuffer@@QEAA@HHW4BufferFlags_t@0@@Z"))); //??0CUtlBuffer@@QEAA@HHH@Z
	bSuccess &= (Functions::fnUtlBufferInit != nullptr);

	Functions::fnUtlBufferPutString = reinterpret_cast<decltype(Functions::fnUtlBufferPutString)>(GetExportAddress(hTier0, X("?PutString@CUtlBuffer@@QEAAXPEBD@Z"))); //ok
	bSuccess &= (Functions::fnUtlBufferPutString != nullptr);

	Functions::fnUtlBufferEnsureCapacity = reinterpret_cast<decltype(Functions::fnUtlBufferEnsureCapacity)>(GetExportAddress(hTier0, X("?EnsureCapacity@CUtlBuffer@@QEAAXH@Z"))); //ok
	bSuccess &= (Functions::fnUtlBufferEnsureCapacity != nullptr);

	Functions::fnUtlStringSet = reinterpret_cast<decltype(Functions::fnUtlStringSet)>(GetExportAddress(hTier0, X("?Set@CUtlString@@QEAAXPEBD@Z"))); //ok
	bSuccess &= (Functions::fnUtlStringSet != nullptr);

	Functions::fnPurgeCBufferString = reinterpret_cast<decltype(Functions::fnPurgeCBufferString)>(GetExportAddress(hTier0, X("?Purge@CBufferString@@QEAAXH@Z"))); //ok
	bSuccess &= (Functions::fnPurgeCBufferString != nullptr);

	return bSuccess;
}

/*
 * overload global new/delete operators with our allocators
 * - @note: ensure that all sdk classes that can be instantiated have an overloaded constructor and/or game allocator, otherwise marked as non-constructible
 */
 // this is a retarded, but easy way of doing it
void* __cdecl operator new(const std::size_t nSize)
{
	if (!Interfaces::m_pMemAlloc)
	{
		const auto pTier0Handle = Memory::GetModuleBaseHandle(TIER0_DLL);
		Interfaces::m_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Memory::GetExportAddress(pTier0Handle, X("g_pMemAlloc")));
	}

	return Interfaces::m_pMemAlloc->Alloc(nSize);
}

void* __cdecl operator new[](const std::size_t nSize)
{
	if (!Interfaces::m_pMemAlloc)
	{
		const auto pTier0Handle = Memory::GetModuleBaseHandle(TIER0_DLL);
		Interfaces::m_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Memory::GetExportAddress(pTier0Handle, X("g_pMemAlloc")));
	}

	return Interfaces::m_pMemAlloc->Alloc(nSize);
}

void __cdecl operator delete(void* pMemory) noexcept
{
	if (!Interfaces::m_pMemAlloc)
	{
		const auto pTier0Handle = Memory::GetModuleBaseHandle(TIER0_DLL);
		Interfaces::m_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Memory::GetExportAddress(pTier0Handle, X("g_pMemAlloc")));
	}

	Interfaces::m_pMemAlloc->Free(pMemory);
}

void __cdecl operator delete[](void* pMemory) noexcept
{
	if (!Interfaces::m_pMemAlloc)
	{
		const auto pTier0Handle = Memory::GetModuleBaseHandle(TIER0_DLL);
		Interfaces::m_pMemAlloc = *reinterpret_cast<IMemAlloc**>(Memory::GetExportAddress(pTier0Handle, X("g_pMemAlloc")));
	}

	Interfaces::m_pMemAlloc->Free(pMemory);
}
#pragma endregion

// @todo: move to win.cpp (or platform.cpp?) except getsectioninfo
#pragma region memory_get

void* Memory::GetModuleBaseHandle(const wchar_t* wszModuleName)
{
	const _PEB* pPEB = reinterpret_cast<_PEB*>(__readgsqword(0x60));

	if (wszModuleName == nullptr)
		return pPEB->ImageBaseAddress;

	void* pModuleBase = nullptr;
	for (LIST_ENTRY* pListEntry = pPEB->Ldr->InMemoryOrderModuleList.Flink; pListEntry != &pPEB->Ldr->InMemoryOrderModuleList; pListEntry = pListEntry->Flink)
	{
		const _LDR_DATA_TABLE_ENTRY* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

		if (pEntry->FullDllName.Buffer != nullptr && CRT::StringCompare(wszModuleName, pEntry->BaseDllName.Buffer) == 0)
		{
			pModuleBase = pEntry->DllBase;
			break;
		}
	}

	if (pModuleBase == nullptr)
	{
		//Logging::PushConsoleColor( FOREGROUND_INTENSE_RED );
		//Logging::Print( X( "[error] module base not found : {}\n" ), wszModuleName );
		//Logging::PopConsoleColor( );
	}

	return pModuleBase;
}

const wchar_t* Memory::GetModuleBaseFileName(const void* hModuleBase)
{
	const _PEB* pPEB = reinterpret_cast<_PEB*>(LI_FN(__readgsqword)(0x60));

	if (hModuleBase == nullptr)
		hModuleBase = pPEB->ImageBaseAddress;

	LI_FN(::EnterCriticalSection)(pPEB->LoaderLock);

	const wchar_t* wszModuleName = nullptr;
	for (LIST_ENTRY* pListEntry = pPEB->Ldr->InMemoryOrderModuleList.Flink; pListEntry != &pPEB->Ldr->InMemoryOrderModuleList; pListEntry = pListEntry->Flink)
	{
		const _LDR_DATA_TABLE_ENTRY* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

		if (pEntry->DllBase == hModuleBase)
		{
			wszModuleName = pEntry->BaseDllName.Buffer;
			break;
		}
	}

	LI_FN(::LeaveCriticalSection)(pPEB->LoaderLock);

	return wszModuleName;
}

void* Memory::GetExportAddress(const void* hModuleBase, const char* szProcedureName)
{
	const auto pBaseAddress = static_cast<const std::uint8_t*>(hModuleBase);

	const auto pIDH = static_cast<const IMAGE_DOS_HEADER*>(hModuleBase);
	if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	const auto pINH = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBaseAddress + pIDH->e_lfanew);
	if (pINH->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	const IMAGE_OPTIONAL_HEADER64* pIOH = &pINH->OptionalHeader;
	const std::uintptr_t nExportDirectorySize = pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	const std::uintptr_t uExportDirectoryAddress = pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	if (nExportDirectorySize == 0U || uExportDirectoryAddress == 0U)
	{
		//Logging::PushConsoleColor( FOREGROUND_INTENSE_RED );
		//Logging::Print( X( "[ error ] module has no exports: {}" ), GetModuleBaseFileName( hModuleBase ) );
		//Logging::PopConsoleColor( );
		return nullptr;
	}

	const auto pIED = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBaseAddress + uExportDirectoryAddress);
	const auto pNamesRVA = reinterpret_cast<const std::uint32_t*>(pBaseAddress + pIED->AddressOfNames);
	const auto pNameOrdinalsRVA = reinterpret_cast<const std::uint16_t*>(pBaseAddress + pIED->AddressOfNameOrdinals);
	const auto pFunctionsRVA = reinterpret_cast<const std::uint32_t*>(pBaseAddress + pIED->AddressOfFunctions);

	// Perform binary search to find the export by name
	std::size_t nRight = pIED->NumberOfNames, nLeft = 0U;
	while (nRight != nLeft)
	{
		// Avoid INT_MAX/2 overflow
		const std::size_t uMiddle = nLeft + ((nRight - nLeft) >> 1U);
		const int iResult = CRT::StringCompare(szProcedureName, reinterpret_cast<const char*>(pBaseAddress + pNamesRVA[uMiddle]));

		if (iResult == 0)
		{
			const std::uint32_t uFunctionRVA = pFunctionsRVA[pNameOrdinalsRVA[uMiddle]];

			//#ifdef _DEBUG
			//			Logging::PushConsoleColor( FOREGROUND_INTENSE_GREEN );
			//			Logging::Print( X( "export found: {} in {} at {}" ), reinterpret_cast< const char* >( pBaseAddress + pNamesRVA[ uMiddle ] ), GetModuleBaseFileName( hModuleBase ), uFunctionRVA );
			//			Logging::PopConsoleColor( );
			//#else
			//			Logging::PushConsoleColor( FOREGROUND_INTENSE_GREEN );
			//			Logging::Print( X( "export found: {}" ), szProcedureName );
			//			Logging::PopConsoleColor( );
			//#endif // _DEBUG

						// Check if it's a forwarded export
			if (uFunctionRVA >= uExportDirectoryAddress && uFunctionRVA - uExportDirectoryAddress < nExportDirectorySize)
			{
				// Forwarded exports are not supported
				break;
			}

			return const_cast<std::uint8_t*>(pBaseAddress) + uFunctionRVA;
		}

		if (iResult > 0)
			nLeft = uMiddle + 1;
		else
			nRight = uMiddle;
	}

	Logging::PushConsoleColor(FOREGROUND_INTENSE_RED);
	Logging::Print(X("[ error ] export not found: {}"), szProcedureName);
	Logging::PopConsoleColor();

	// Export not found
	return nullptr;
}

bool Memory::GetSectionInfo(const void* hModuleBase, const char* szSectionName, std::uint8_t** ppSectionStart, std::size_t* pnSectionSize)
{
	const auto pBaseAddress = static_cast<const std::uint8_t*>(hModuleBase);

	const auto pIDH = static_cast<const IMAGE_DOS_HEADER*>(hModuleBase);
	if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	const auto pINH = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBaseAddress + pIDH->e_lfanew);
	if (pINH->Signature != IMAGE_NT_SIGNATURE)
		return false;

	const IMAGE_SECTION_HEADER* pISH = IMAGE_FIRST_SECTION(pINH);

	// go through all code sections
	for (WORD i = 0U; i < pINH->FileHeader.NumberOfSections; i++, pISH++)
	{
		// @test: use case insensitive comparison instead?
		if (CRT::StringCompareN(szSectionName, reinterpret_cast<const char*>(pISH->Name), IMAGE_SIZEOF_SHORT_NAME) == 0)
		{
			if (ppSectionStart != nullptr)
				*ppSectionStart = const_cast<std::uint8_t*>(pBaseAddress) + pISH->VirtualAddress;

			if (pnSectionSize != nullptr)
				*pnSectionSize = pISH->SizeOfRawData;

			return true;
		}
	}

	Logging::PushConsoleColor(FOREGROUND_INTENSE_RED);
	Logging::Print(X("[ error ] code section not found: {}"), szSectionName);
	Logging::PopConsoleColor();

	return false;
}

typedef struct _LDR_MODULE
{
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
} LDR_MODULE, * PLDR_MODULE;

typedef struct _PEB_LOADER_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LOADER_DATA, * PPEB_LOADER_DATA;

struct pattern_token_t
{
	uint8_t byte;
	bool wc;
};

__forceinline uint8_t hex_to_char(const char in)
{
	return static_cast<char>(isdigit(in) ? (in - '0') : (in - 'A' + 10));
}

struct module_t
{
	uintptr_t base;
	uint32_t size;
};

static auto file_name_w = [](wchar_t* path)
	{
		wchar_t* slash = path;

		while (path && *path)
		{
			if ((*path == '\\' || *path == '/' || *path == ':') && path[1] && path[1] != '\\' && path[1] != '/')
				slash = path + 1;
			path++;
		}

		return slash;
	};

//std::optional<module_t> find_module(const wchar_t* wszModuleName)
//{
//	static std::unordered_map<std::wstring, module_t> cache;
//	static std::mutex mutex;
//
//	std::wstring inputName(wszModuleName);
//	std::ranges::transform(inputName, inputName.begin(), ::towlower);
//
//	{
//		std::lock_guard lock(mutex);
//		auto it = cache.find(inputName);
//		if (it != cache.end())
//			return it->second;
//	}
//
//	const auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;
//	if (!peb)
//		return {};
//
//	const auto ldr = reinterpret_cast<PEB_LOADER_DATA*>(peb->Ldr);
//	if (!ldr)
//		return {};
//
//	const auto head = &ldr->InLoadOrderModuleList;
//	auto cur = head->Flink;
//
//	while (cur != head)
//	{
//		const auto mod = CONTAINING_RECORD(cur, LDR_MODULE, InLoadOrderModuleList);
//
//		std::wstring wide(mod->BaseDllName.Buffer, mod->BaseDllName.Length / sizeof(wchar_t));
//		std::ranges::transform(wide, wide.begin(), ::towlower);
//
//		if (wide == inputName)
//		{
//			module_t result{ reinterpret_cast<uintptr_t>(mod->BaseAddress), mod->SizeOfImage };
//
//			std::lock_guard lock(mutex);
//			cache[inputName] = result;
//			return result;
//		}
//
//		cur = cur->Flink;
//	}
//
//	return {};
//}
//
//std::uint8_t* Memory::FindPattern(const wchar_t* wszModuleName, const char* szPattern)
//{
//	const auto mod = find_module(wszModuleName);
//	if (!mod)
//	{
//#ifdef _DEBUG
//		printf("module not found: %s\n", szPattern);
//#endif
//		return 0;
//	}
//
//	std::vector<pattern_token_t> tokens;
//	std::optional<uint8_t> cur{};
//	for (int i{}; i < strlen(szPattern); ++i)
//	{
//		if (szPattern[i] == ' ')
//			continue;
//
//		if (szPattern[i] != '?')
//		{
//			if (cur)
//			{
//				tokens.emplace_back(pattern_token_t{ static_cast<uint8_t>(*cur * 16 + hex_to_char(szPattern[i])), false });
//				cur = {};
//				continue;
//			}
//
//			cur = hex_to_char(szPattern[i]);
//		}
//		else
//		{
//			cur = {};
//			tokens.emplace_back(pattern_token_t{ 0, true });
//		}
//	}
//
//	const auto compare = [&](uintptr_t addr)
//		{
//			for (auto& [b, w] : tokens)
//			{
//				if (w)
//				{
//					addr++;
//					continue;
//				}
//
//				if (b != *reinterpret_cast<uint8_t*>(addr++))
//					return false;
//			}
//
//			return true;
//		};
//
//	for (uintptr_t i{ mod->base }; i < static_cast<uintptr_t>(mod->base + mod->size); ++i)
//		if (compare(i))
//		{
//				return (std::uint8_t*)i;
//		}
//
//#ifdef _DEBUG
//	printf("pattern not found: %s\n", szPattern);
//#endif
//
//	return 0;
//}

std::uint8_t* Memory::FindPattern(const wchar_t* wszModuleName, const char* szPattern)
{
	// convert pattern string to byte array
	const std::size_t nApproximateBufferSize = (CRT::StringLength(szPattern) >> 1U) + 1U;
	std::uint8_t* arrByteBuffer = static_cast<std::uint8_t*>(MEM_STACKALLOC(nApproximateBufferSize));
	char* szMaskBuffer = static_cast<char*>(MEM_STACKALLOC(nApproximateBufferSize));
	PatternToBytes(szPattern, arrByteBuffer, szMaskBuffer);

	// @test: use search with straight in-place conversion? do not think it will be faster, cuz of bunch of new checks that gonna be performed for each iteration
	return FindPattern(wszModuleName, reinterpret_cast<const char*>(arrByteBuffer), szMaskBuffer);
}

std::uint8_t* Memory::FindPattern(const wchar_t* wszModuleName, const char* szBytePattern, const char* szByteMask)
{
	const void* hModuleBase = GetModuleBaseHandle(wszModuleName);

	if (hModuleBase == nullptr)
	{
		//Logging::PushConsoleColor( FOREGROUND_INTENSE_RED );
		//Logging::Print( X( "[ error ] failed to get module handle for: {}" ), wszModuleName );
		//Logging::PopConsoleColor( );
		return nullptr;
	}

	const auto pBaseAddress = static_cast<const std::uint8_t*>(hModuleBase);
	const auto pIDH = static_cast<const IMAGE_DOS_HEADER*>(hModuleBase);
	if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
	{
		Logging::PushConsoleColor(FOREGROUND_INTENSE_RED);
		Logging::Print(X("[ error ] failed to get module size, image is invalid"));
		Logging::PopConsoleColor();
		return nullptr;
	}

	const auto pINH = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBaseAddress + pIDH->e_lfanew);
	if (pINH->Signature != IMAGE_NT_SIGNATURE)
	{
		Logging::PushConsoleColor(FOREGROUND_INTENSE_RED);
		Logging::Print(X("[ error ] failed to get module size, image is invalid"));
		Logging::PopConsoleColor();
		return nullptr;
	}

	const std::uint8_t* arrByteBuffer = reinterpret_cast<const std::uint8_t*>(szBytePattern);
	const std::size_t nByteCount = CRT::StringLength(szByteMask);

	std::uint8_t* pFoundAddress = nullptr;

	// @todo: we also can go through code sections and skip noexec pages, but will it really improve performance? / or at least for all occurrences search
	// https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_section_header

	pFoundAddress = FindPatternEx(pBaseAddress, pINH->OptionalHeader.SizeOfImage, arrByteBuffer, nByteCount, szByteMask);

	if (pFoundAddress == nullptr)
	{
		char* szPattern = static_cast<char*>(MEM_STACKALLOC((nByteCount << 1U) + nByteCount));
		[[maybe_unused]] const std::size_t nConvertedPatternLength = BytesToPattern(arrByteBuffer, nByteCount, szPattern);

		Logging::PushConsoleColor(FOREGROUND_INTENSE_RED);
		Logging::Print(X("[ error ] pattern not found: {}"), szPattern);
		Logging::PopConsoleColor();
		MEM_STACKFREE(szPattern);
	}

	return pFoundAddress;
}
std::uint8_t* Memory::FindPatternEx(
	const std::uint8_t* pRegionStart,
	std::size_t nRegionSize,
	const std::uint8_t* arrByteBuffer,
	std::size_t nByteCount,
	const char* szByteMask)
{
	const std::uint8_t* end = pRegionStart + nRegionSize - nByteCount;

	for (const std::uint8_t* cur = pRegionStart; cur <= end; ++cur)
	{
		bool matched = true;

		for (std::size_t i = 0; i < nByteCount; i++)
		{
			if (szByteMask[i] != '?' && cur[i] != arrByteBuffer[i])
			{
				matched = false;
				break;
			}
		}

		if (matched)
			return const_cast<std::uint8_t*>(cur);
	}

	return nullptr;
}


std::vector<std::uint8_t*> Memory::FindPatternAllOccurrencesEx(const std::uint8_t* pRegionStart, const std::size_t nRegionSize, const std::uint8_t* arrByteBuffer, const std::size_t nByteCount, const char* szByteMask)
{
	const std::uint8_t* pRegionEnd = pRegionStart + nRegionSize - nByteCount;
	const bool bIsMaskUsed = (szByteMask != nullptr);

	// container for addresses of the all found occurrences
	std::vector<std::uint8_t*> vecOccurrences = {};

	for (std::uint8_t* pCurrentByte = const_cast<std::uint8_t*>(pRegionStart); pCurrentByte < pRegionEnd; ++pCurrentByte)
	{
		// do a first byte check before entering the loop, otherwise if there two consecutive bytes of first byte in the buffer, we may skip both and fail the search
		if ((!bIsMaskUsed || *szByteMask != '?') && *pCurrentByte != *arrByteBuffer)
			continue;

		// check for bytes sequence match
		bool bSequenceMatch = true;
		for (std::size_t i = 1U; i < nByteCount; i++)
		{
			// compare sequence and continue on wildcard or skip forward on first mismatched byte
			if ((!bIsMaskUsed || szByteMask[i] != '?') && pCurrentByte[i] != arrByteBuffer[i])
			{
				// skip non suitable bytes
				pCurrentByte += i - 1U;

				bSequenceMatch = false;
				break;
			}
		}

		// check did we found address
		if (bSequenceMatch)
			vecOccurrences.emplace_back(pCurrentByte);
	}

	return vecOccurrences;
}

std::size_t Memory::PatternToBytes(const char* szPattern, std::uint8_t* pOutByteBuffer, char* szOutMaskBuffer)
{
	std::uint8_t* pCurrentByte = pOutByteBuffer;

	while (*szPattern != '\0')
	{
		// check is a wildcard
		if (*szPattern == '?')
		{
			++szPattern;

			// ignore that
			*pCurrentByte++ = 0U;
			*szOutMaskBuffer++ = '?';
		}
		// check is not space
		else if (*szPattern != ' ')
		{
			// convert two consistent numbers in a row to byte value
			std::uint8_t uByte = static_cast<std::uint8_t>(CRT::CharToHexInt(*szPattern) << 4);

			++szPattern;

			uByte |= static_cast<std::uint8_t>(CRT::CharToHexInt(*szPattern));

			*pCurrentByte++ = uByte;
			*szOutMaskBuffer++ = 'x';
		}

		++szPattern;
	}

	// zero terminate both buffers
	*pCurrentByte = 0U;
	*szOutMaskBuffer = '\0';

	return pCurrentByte - pOutByteBuffer;
}

std::size_t Memory::BytesToPattern(const std::uint8_t* pByteBuffer, const std::size_t nByteCount, char* szOutBuffer)
{
	char* szCurrentPattern = szOutBuffer;

	for (std::size_t i = 0U; i < nByteCount; i++)
	{
		// manually convert byte to chars
		const char* szHexByte = &CRT::_TWO_DIGITS_HEX_LUT[pByteBuffer[i] * 2U];
		*szCurrentPattern++ = szHexByte[0];
		*szCurrentPattern++ = szHexByte[1];
		*szCurrentPattern++ = ' ';
	}
	*--szCurrentPattern = '\0';

	return szCurrentPattern - szOutBuffer;
}

void* CS2ChannelManager::RegisterChannel(const char* szName)
{
	using fn_t = void* (*)(const char*, uintptr_t, int, int, uint32_t);
	static fn_t fn = (fn_t)Memory::GetExportAddress(Memory::GetModuleBaseHandle(L"tier0.dll"), "LoggingSystem_RegisterLoggingChannel");
	return fn(szName, 0, 0, 2, 0xFFFFFFFF);
}

void* CS2ChannelManager::GetChannel(const char* szName)
{
	static std::unordered_map<FNV1A_t, void*> mapChannels{};

	if (!mapChannels.contains(FNV1A::Hash(szName)))
		mapChannels.insert({ FNV1A::Hash(szName), CS2ChannelManager::RegisterChannel(szName) });

	return mapChannels[FNV1A::Hash(szName)];
}

void CS2ChannelManager::Msg(const char* szChannelName, const char* szMsg, Color nColor)
{
	void* pChannel = CS2ChannelManager::GetChannel(szChannelName);
	using fn_t = void* (*)(void*, int, Color, const char*);
	static fn_t fn = (fn_t)Memory::GetExportAddress(Memory::GetModuleBaseHandle(L"tier0.dll"), "?LoggingSystem_Log@@YA?AW4LoggingResponse_t@@HW4LoggingSeverity_t@@VColor@@PEBDZZ");
	fn(pChannel, 2, nColor, (std::string(szMsg) + "\n").c_str());
}