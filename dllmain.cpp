#include "stdafx.h"
#include <Windows.h>

//we are hijacking dll XmlLite so define where original one is located
#define ORIG_DLL ("C:\\Windows\\System32\\XmlLite.dll")

typedef void(*fnDummy)(); //return value or parameters are unimportant, we just want it to jump
fnDummy gCreateXmlReader;
fnDummy gCreateXmlReaderInputWithEncodingCodePage;
fnDummy gCreateXmlReaderInputWithEncodingName;
fnDummy gCreateXmlWriter;
fnDummy gCreateXmlWriterOutputWithEncodingCodePage;
fnDummy gCreateXmlWriterOutputWithEncodingName;

extern "C" __declspec(dllexport) void CreateXmlReader() //these all should compile as only single instruction functions: jmp qword ptr[...]
{
	gCreateXmlReader();
}
extern "C" __declspec(dllexport) void CreateXmlReaderInputWithEncodingCodePage()
{
	gCreateXmlReaderInputWithEncodingCodePage();
}
extern "C" __declspec(dllexport) void CreateXmlReaderInputWithEncodingName()
{
	gCreateXmlReaderInputWithEncodingName();
}
extern "C" __declspec(dllexport) void CreateXmlWriter()
{
	gCreateXmlWriter();
}
extern "C" __declspec(dllexport) void CreateXmlWriterOutputWithEncodingCodePage()
{
	gCreateXmlWriterOutputWithEncodingCodePage();
}
extern "C" __declspec(dllexport) void CreateXmlWriterOutputWithEncodingName()
{
	gCreateXmlWriterOutputWithEncodingName();
}

fnDummy GetDllExport(LPCSTR DllName, LPCSTR ExportName)
{
	return (fnDummy)GetProcAddress(LoadLibraryA(DllName), ExportName);
}
BOOL WriteToReadOnly(PVOID Address, PVOID Buffer, SIZE_T Size)
{
	return WriteProcessMemory(GetCurrentProcess(), Address, Buffer, Size, 0);
}

BYTE fixCertVerifyTimeValidity[] = {0x48, 0x31, 0xC0, 0xC3}; //xor rax,rax | ret
BYTE fixGetSystemTimeAsFileTime[] = {0x48, 0x83, 0x21, 0x00, 0xC3}; //and qword ptr[rcx],0x00 | ret

BOOL InitHooks()
{
	if ((gCreateXmlReader = GetDllExport(ORIG_DLL, "CreateXmlReader")) &&
		(gCreateXmlReaderInputWithEncodingCodePage = GetDllExport(ORIG_DLL, "CreateXmlReaderInputWithEncodingCodePage")) &&
		(gCreateXmlReaderInputWithEncodingName = GetDllExport(ORIG_DLL, "CreateXmlReaderInputWithEncodingName")) &&
		(gCreateXmlWriter = GetDllExport(ORIG_DLL, "CreateXmlWriter")) &&
		(gCreateXmlWriterOutputWithEncodingCodePage = GetDllExport(ORIG_DLL, "CreateXmlWriterOutputWithEncodingCodePage")) &&
		(gCreateXmlWriterOutputWithEncodingName = GetDllExport(ORIG_DLL, "CreateXmlWriterOutputWithEncodingName")))
	{
		return WriteToReadOnly(GetDllExport("crypt32.dll", "CertVerifyTimeValidity"), fixCertVerifyTimeValidity, sizeof fixCertVerifyTimeValidity) &&
			WriteToReadOnly(GetDllExport("KernelBase.dll", "GetSystemTimeAsFileTime"), fixGetSystemTimeAsFileTime, sizeof fixGetSystemTimeAsFileTime);
	}
	return FALSE;
}

BOOL APIENTRY entry(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		return InitHooks();
	return TRUE;
}
