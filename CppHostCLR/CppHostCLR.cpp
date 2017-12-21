#pragma region Includes and Imports
#include <stdio.h>
#include <windows.h>
#include <metahost.h>
#include <Shlobj.h>

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

	wprintf(L"Load the assembly %s\nargs\n%s\n", pszAssemblyPath, pszStringArg);
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
		pMetaHost = nullptr;
	}
	if (pRuntimeInfo)
	{
		pRuntimeInfo->Release();
		pRuntimeInfo = nullptr;
	}
	if (pClrRuntimeHost)
	{
		wprintf(L"Stop the .NET runtime\n");
		pClrRuntimeHost->Stop();

		pClrRuntimeHost->Release();
		pClrRuntimeHost = nullptr;
	}

	return hr;
}

int GetPathFromRegistry(const wchar_t *wsKey, const wchar_t *wsValueName, wchar_t *wsPath)
{
	HKEY hKey;
	auto lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wsKey, 0L, KEY_READ, &hKey);

	if (lRet != ERROR_SUCCESS)
	{
		return -1;
	}

	DWORD dwDataSize = MAX_PATH;
	DWORD dwType;
	lRet = RegQueryValueEx(hKey, wsValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(wsPath), &dwDataSize);

	if (lRet != ERROR_SUCCESS)
	{
		return -1;
	}

	RegCloseKey(hKey);
	return ERROR_SUCCESS;
}

int count_chars(const wchar_t *&argument)
{
	auto length = wcslen(argument);
	auto count = 0;
	for (auto i = 0; i < length; i++)
	{
		if (argument[i] == '"' /*|| argument[i] == '\\'*/)
			count++;
	}

	return count;
}

wchar_t *escape_quotes(const wchar_t *&argument)
{
	auto length = wcslen(argument);
	auto count = count_chars(argument);
	
	int size = sizeof(wchar_t) * (length + count + 1);
	auto result = static_cast<wchar_t*>(malloc(size));
	memset(result, 0, size);
	for (auto i = 0, j = 0; i <= length; i++, j++) 
	{
		switch (argument[i])
		{
			case '"':
				result[j] = '\\';
				result[j + 1] = '"';
				j++;
				break;
			//case '\\':
			//	result[j] = '\\';
			//	result[j + 1] = '\\';
			//	j++;
			default:
				result[j] = argument[i];
		}
	}

	return result;
}

wchar_t *encapsulate_args(int argc, const wchar_t *argv[])
{
	auto length = 0;
	for (auto i = 1; i < argc; i++)
	{
		length += wcslen(argv[i]) + 3;
		length += count_chars(argv[i]);
	}

	int size = sizeof(wchar_t) * (length + argc);
	auto data = static_cast<wchar_t*>(malloc(size));
	memset(data, 0, size);
	auto database = data;
	for (auto i = 1; i < argc; i++)
	{
		auto escaped = escape_quotes(argv[i]);
		length = wcslen(escaped);
		wsprintf(data, L"\"%s\" ", escaped);
		data += length + 3;
	}

	return database;
}

int wmain(int argc, const wchar_t *argv[])
{
	wchar_t wsRegPath[MAX_PATH];
	wchar_t wsBlPath[MAX_PATH];
	wchar_t commonFilesPath[MAX_PATH];
	wchar_t openSslPath[MAX_PATH];

	if (!SHGetSpecialFolderPath(nullptr, commonFilesPath, CSIDL_PROGRAM_FILES_COMMONX86, FALSE))
	{
		wprintf(L"Couldn't get common files directory.\n");
		return -1;
	}

#ifdef _WIN64
	auto lRet = GetPathFromRegistry(L"SOFTWARE\\WOW6432Node\\SolarWinds\\Orion\\Core", L"InstallPath", wsRegPath);
	wsprintf(wsBlPath, L"%sSolarWinds.BusinessLayerHostx64.exe", wsRegPath);
	wsprintf(openSslPath, L"%s\\SolarWinds\\OpenSSL\\x64\\", commonFilesPath);
#else
	auto lRet = GetPathFromRegistry(L"SOFTWARE\\SolarWinds\\Orion\\Core", L"InstallPath", wsRegPath);
	wsprintf(wsBlPath, L"%sSolarWinds.BusinessLayerHost.exe", wsRegPath);
	wsprintf(openSslPath, L"%s\\SolarWinds\\OpenSSL\\x86\\", commonFilesPath);
#endif
	if (lRet != ERROR_SUCCESS)
	{
		wprintf(L"Couldn't read InstallPath from Windows registry.\n");
		return -1;
	}

	if (!SetDllDirectory(openSslPath))
	{
		wprintf(L"Couldn't set dll directory.\n");
		return -1;
	}

	if (nullptr == LoadLibrary(L"libeay32.dll"))
	{
		wprintf(L"Couldn't load libeay32.dll.\n");
		return -1;
	}

	auto encapsulated_args = encapsulate_args(argc, argv);
    return RuntimeHostV4(L"v4.0.30319", wsBlPath, L"SolarWinds.BusinessLayerHost.Program", L"Start", encapsulated_args);
}