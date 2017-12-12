#pragma region Includes and Imports
#include <stdio.h>
#include <windows.h>
#include <metahost.h>

#pragma comment(lib, "mscoree.lib")

#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;
#pragma endregion

HRESULT RuntimeHostV4(PCWSTR pszVersion, PCWSTR pszAssemblyPath,
	PCWSTR pszClassName, PCWSTR pszStaticMethodName, PCWSTR pszStringArg)
{
	HRESULT hr;
	ICLRMetaHost *pMetaHost = NULL;
	ICLRRuntimeInfo *pRuntimeInfo = NULL;
	ICLRRuntimeHost *pClrRuntimeHost = NULL;

	wprintf(L"Load and start the .NET runtime %s \n", pszVersion);
	hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&pMetaHost));
	if (FAILED(hr))
	{
		wprintf(L"CLRCreateInstance failed w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}
	hr = pMetaHost->GetRuntime(pszVersion, IID_PPV_ARGS(&pRuntimeInfo));
	if (FAILED(hr))
	{
		wprintf(L"ICLRMetaHost::GetRuntime failed w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	BOOL fLoadable;
	hr = pRuntimeInfo->IsLoadable(&fLoadable);
	if (FAILED(hr))
	{
		wprintf(L"ICLRRuntimeInfo::IsLoadable failed w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}
	if (!fLoadable)
	{
		wprintf(L".NET runtime %s cannot be loaded\n", pszVersion);
		goto Cleanup;
	}

	hr = pRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost,
		IID_PPV_ARGS(&pClrRuntimeHost));
	if (FAILED(hr))
	{
		wprintf(L"ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	hr = pClrRuntimeHost->Start();
	if (FAILED(hr))
	{
		wprintf(L"CLR failed to start w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	wprintf(L"Load the assembly %s\n", pszAssemblyPath);
	DWORD returnValue = 0;
	hr = pClrRuntimeHost->ExecuteInDefaultAppDomain(pszAssemblyPath,
		pszClassName, pszStaticMethodName, pszStringArg, &returnValue);
	if (FAILED(hr))
	{
		wprintf(L"Failed to call method w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	wprintf(L"Call %s.%s(\"%s\") => %d\n", pszClassName, pszStaticMethodName,
		pszStringArg, returnValue);

Cleanup:

	if (pMetaHost)
	{
		pMetaHost->Release();
		pMetaHost = NULL;
	}
	if (pRuntimeInfo)
	{
		pRuntimeInfo->Release();
		pRuntimeInfo = NULL;
	}
	if (pClrRuntimeHost)
	{
		//wprintf(L"Stop the .NET runtime\n");
		//pClrRuntimeHost->Stop();

		pClrRuntimeHost->Release();
		pClrRuntimeHost = NULL;
	}

	return hr;
}

int wmain(int argc, const wchar_t *argv[])
{
	wchar_t* data = NULL;
	int length = 0;
	for (int i = 1; i < argc; i++)
	{
		length += wcslen(argv[i]) + 3;
	}

	int size = sizeof(wchar_t) * (length + argc);
	data = (wchar_t*)malloc(size);
	memset(data, 0, size);
	wchar_t *database = data;
	for (int i = 1; i < argc; i++)
	{
		length = wcslen(argv[i]);
		wsprintf(data, L"\"%s\" ", argv[i]);
		data += length + 3;
	}

    RuntimeHostV4(L"v4.0.30319", L"ConsoleApp1.exe", L"ConsoleApp1.Program", L"Start", database);

    return 0;
}