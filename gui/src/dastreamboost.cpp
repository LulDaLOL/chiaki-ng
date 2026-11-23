// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "dastreamboost.h"

#include <windows.h>

#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

// Optional system DLLs (wlanapi, winmm) and newer kernel32 functions are loaded at runtime,
// so the app still starts on PCs without Wi-Fi or on older Windows builds. Only the few
// documented values used here are declared, to avoid depending on extra SDK headers.
namespace
{
typedef DWORD (WINAPI *WlanOpenHandleFn)(DWORD, PVOID, PDWORD, PHANDLE);
typedef DWORD (WINAPI *WlanCloseHandleFn)(HANDLE, PVOID);
typedef DWORD (WINAPI *WlanEnumInterfacesFn)(HANDLE, PVOID, PVOID *);
typedef DWORD (WINAPI *WlanSetInterfaceFn)(HANDLE, const GUID *, DWORD, DWORD, PVOID, PVOID);
typedef VOID (WINAPI *WlanFreeMemoryFn)(PVOID);
typedef UINT (WINAPI *TimePeriodFn)(UINT);
typedef BOOL (WINAPI *SetProcessInformationFn)(HANDLE, int, LPVOID, DWORD);

constexpr DWORD kWlanClientVersion = 2;
constexpr DWORD kOpBackgroundScanEnabled = 2; // wlan_intf_opcode_background_scan_enabled (BOOL)
constexpr DWORD kOpMediaStreamingMode = 3;    // wlan_intf_opcode_media_streaming_mode (BOOL)
constexpr DWORD kStateConnected = 1;          // wlan_interface_state_connected
// WLAN_INTERFACE_INFO_LIST: DWORD count, DWORD index, then WLAN_INTERFACE_INFO entries of
// GUID (16) + WCHAR[256] description (512) + WLAN_INTERFACE_STATE (4) = 532 bytes each.
constexpr size_t kListHeaderSize = 8;
constexpr size_t kInterfaceInfoSize = 532;
constexpr size_t kInterfaceStateOffset = 528;
constexpr DWORD kMaxInterfaces = 64;

constexpr int kProcessPowerThrottling = 4;          // PROCESS_INFORMATION_CLASS::ProcessPowerThrottling
constexpr ULONG kPowerThrottlingVersion = 1;        // PROCESS_POWER_THROTTLING_CURRENT_VERSION
constexpr ULONG kPowerThrottlingExecutionSpeed = 1; // PROCESS_POWER_THROTTLING_EXECUTION_SPEED
struct PowerThrottlingState
{
	ULONG version;
	ULONG control_mask;
	ULONG state_mask;
};

struct Boost
{
	bool active = false;

	HMODULE wlan_dll = nullptr;
	HANDLE wlan_handle = nullptr;
	WlanCloseHandleFn wlan_close = nullptr;
	WlanSetInterfaceFn wlan_set = nullptr;
	std::vector<GUID> wlan_interfaces;

	HMODULE winmm_dll = nullptr;
	TimePeriodFn time_end_period = nullptr;

	DWORD old_priority_class = 0;
	SetProcessInformationFn set_process_information = nullptr;
};

std::mutex g_mutex;
Boost g_boost;

void WlanSetBool(WlanSetInterfaceFn set, HANDLE handle, const GUID &iface, DWORD opcode, BOOL value)
{
	BOOL data = value;
	set(handle, &iface, opcode, static_cast<DWORD>(sizeof(data)), &data, nullptr);
}

void StartWifi(Boost &boost)
{
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
	if(open(kWlanClientVersion, nullptr, &negotiated, &handle) != ERROR_SUCCESS || !handle)
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
		WlanSetBool(set, handle, iface, kOpBackgroundScanEnabled, FALSE);
		WlanSetBool(set, handle, iface, kOpMediaStreamingMode, TRUE);
	}
	boost.wlan_dll = dll;
	boost.wlan_handle = handle;
	boost.wlan_close = close;
	boost.wlan_set = set;
	boost.wlan_interfaces = std::move(connected);
}

void StopWifi(Boost &boost)
{
	if(!boost.wlan_handle)
		return;
	for(const GUID &iface : boost.wlan_interfaces)
	{
		WlanSetBool(boost.wlan_set, boost.wlan_handle, iface, kOpMediaStreamingMode, FALSE);
		WlanSetBool(boost.wlan_set, boost.wlan_handle, iface, kOpBackgroundScanEnabled, TRUE);
	}
	boost.wlan_close(boost.wlan_handle, nullptr);
	FreeLibrary(boost.wlan_dll);
	boost.wlan_dll = nullptr;
	boost.wlan_handle = nullptr;
	boost.wlan_interfaces.clear();
}

void StartTimer(Boost &boost)
{
	HMODULE dll = LoadLibraryW(L"winmm.dll");
	if(!dll)
		return;
	auto begin = reinterpret_cast<TimePeriodFn>(GetProcAddress(dll, "timeBeginPeriod"));
	auto end = reinterpret_cast<TimePeriodFn>(GetProcAddress(dll, "timeEndPeriod"));
	if(!begin || !end || begin(1) != 0) // 0 = TIMERR_NOERROR
	{
		FreeLibrary(dll);
		return;
	}
	boost.winmm_dll = dll;
	boost.time_end_period = end;
}

void StopTimer(Boost &boost)
{
	if(!boost.winmm_dll)
		return;
	boost.time_end_period(1);
	FreeLibrary(boost.winmm_dll);
	boost.winmm_dll = nullptr;
	boost.time_end_period = nullptr;
}

void SetPowerThrottling(SetProcessInformationFn set_info, bool allow)
{
	PowerThrottlingState state;
	state.version = kPowerThrottlingVersion;
	state.control_mask = allow ? 0 : kPowerThrottlingExecutionSpeed; // 0 = let Windows decide again
	state.state_mask = 0;
	set_info(GetCurrentProcess(), kProcessPowerThrottling, &state, static_cast<DWORD>(sizeof(state)));
}

void StartProcess(Boost &boost)
{
	HANDLE self = GetCurrentProcess();
	DWORD old_class = GetPriorityClass(self);
	if(old_class == NORMAL_PRIORITY_CLASS && SetPriorityClass(self, ABOVE_NORMAL_PRIORITY_CLASS))
		boost.old_priority_class = old_class;

	HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
	auto set_info = kernel ? reinterpret_cast<SetProcessInformationFn>(GetProcAddress(kernel, "SetProcessInformation")) : nullptr;
	if(set_info)
	{
		SetPowerThrottling(set_info, false);
		boost.set_process_information = set_info;
	}

	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}

void StopProcess(Boost &boost)
{
	if(boost.old_priority_class)
		SetPriorityClass(GetCurrentProcess(), boost.old_priority_class);
	boost.old_priority_class = 0;
	if(boost.set_process_information)
		SetPowerThrottling(boost.set_process_information, true);
	boost.set_process_information = nullptr;
	SetThreadExecutionState(ES_CONTINUOUS);
}
}

void DaStreamBoostStart()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if(g_boost.active)
		return;
	g_boost.active = true;
	StartWifi(g_boost);
	StartTimer(g_boost);
	StartProcess(g_boost);
}

void DaStreamBoostStop()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if(!g_boost.active)
		return;
	StopWifi(g_boost);
	StopTimer(g_boost);
	StopProcess(g_boost);
	g_boost.active = false;
}
