// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "dawifiboost.h"

#include <windows.h>

#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

// wlanapi.dll is loaded at runtime so the app still starts on PCs without the WLAN service.
// Only the few documented values used here are declared, to avoid depending on wlanapi.h.
namespace
{
typedef DWORD (WINAPI *WlanOpenHandleFn)(DWORD, PVOID, PDWORD, PHANDLE);
typedef DWORD (WINAPI *WlanCloseHandleFn)(HANDLE, PVOID);
typedef DWORD (WINAPI *WlanEnumInterfacesFn)(HANDLE, PVOID, PVOID *);
typedef DWORD (WINAPI *WlanSetInterfaceFn)(HANDLE, const GUID *, DWORD, DWORD, PVOID, PVOID);
typedef VOID (WINAPI *WlanFreeMemoryFn)(PVOID);

constexpr DWORD kClientVersion = 2;
constexpr DWORD kOpBackgroundScanEnabled = 2; // wlan_intf_opcode_background_scan_enabled (BOOL)
constexpr DWORD kOpMediaStreamingMode = 3;    // wlan_intf_opcode_media_streaming_mode (BOOL)
constexpr DWORD kStateConnected = 1;          // wlan_interface_state_connected
// WLAN_INTERFACE_INFO_LIST: DWORD count, DWORD index, then WLAN_INTERFACE_INFO entries of
// GUID (16) + WCHAR[256] description (512) + WLAN_INTERFACE_STATE (4) = 532 bytes each.
constexpr size_t kListHeaderSize = 8;
constexpr size_t kInterfaceInfoSize = 532;
constexpr size_t kInterfaceStateOffset = 528;
constexpr DWORD kMaxInterfaces = 64;

struct Boost
{
	HMODULE dll = nullptr;
	HANDLE handle = nullptr;
	WlanCloseHandleFn close = nullptr;
	WlanSetInterfaceFn set = nullptr;
	std::vector<GUID> interfaces;
};

std::mutex g_mutex;
Boost g_boost;

void SetBool(WlanSetInterfaceFn set, HANDLE handle, const GUID &iface, DWORD opcode, BOOL value)
{
	BOOL data = value;
	set(handle, &iface, opcode, static_cast<DWORD>(sizeof(data)), &data, nullptr);
}
}

void DaWifiBoostStart()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if(g_boost.handle)
		return;

	HMODULE dll = LoadLibraryW(L"wlanapi.dll");
	if(!dll)
		return;
	auto open = reinterpret_cast<WlanOpenHandleFn>(GetProcAddress(dll, "WlanOpenHandle"));
	auto close = reinterpret_cast<WlanCloseHandleFn>(GetProcAddress(dll, "WlanCloseHandle"));
	auto enum_ifaces = reinterpret_cast<WlanEnumInterfacesFn>(GetProcAddress(dll, "WlanEnumInterfaces"));
	auto set = reinterpret_cast<WlanSetInterfaceFn>(GetProcAddress(dll, "WlanSetInterface"));
	auto free_mem = reinterpret_cast<WlanFreeMemoryFn>(GetProcAddress(dll, "WlanFreeMemory"));
	if(!open || !close || !enum_ifaces || !set || !free_mem)
	{
		FreeLibrary(dll);
		return;
	}

	DWORD negotiated = 0;
	HANDLE handle = nullptr;
	if(open(kClientVersion, nullptr, &negotiated, &handle) != ERROR_SUCCESS || !handle)
	{
		FreeLibrary(dll);
		return;
	}

	std::vector<GUID> connected;
	PVOID list = nullptr;
	if(enum_ifaces(handle, nullptr, &list) == ERROR_SUCCESS && list)
	{
		const BYTE *base = static_cast<const BYTE *>(list);
		DWORD count = 0;
		std::memcpy(&count, base, sizeof(count));
		for(DWORD i = 0; i < count && i < kMaxInterfaces; i++)
		{
			const BYTE *item = base + kListHeaderSize + i * kInterfaceInfoSize;
			DWORD state = 0;
			std::memcpy(&state, item + kInterfaceStateOffset, sizeof(state));
			if(state != kStateConnected)
				continue;
			GUID id;
			std::memcpy(&id, item, sizeof(id));
			connected.push_back(id);
		}
		free_mem(list);
	}

	if(connected.empty()) // wired or offline: nothing to do
	{
		close(handle, nullptr);
		FreeLibrary(dll);
		return;
	}

	for(const GUID &iface : connected)
	{
		SetBool(set, handle, iface, kOpBackgroundScanEnabled, FALSE);
		SetBool(set, handle, iface, kOpMediaStreamingMode, TRUE);
	}

	g_boost.dll = dll;
	g_boost.handle = handle;
	g_boost.close = close;
	g_boost.set = set;
	g_boost.interfaces = std::move(connected);
}

void DaWifiBoostStop()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if(!g_boost.handle)
		return;
	for(const GUID &iface : g_boost.interfaces)
	{
		SetBool(g_boost.set, g_boost.handle, iface, kOpMediaStreamingMode, FALSE);
		SetBool(g_boost.set, g_boost.handle, iface, kOpBackgroundScanEnabled, TRUE);
	}
	g_boost.close(g_boost.handle, nullptr);
	FreeLibrary(g_boost.dll);
	g_boost = Boost();
}
