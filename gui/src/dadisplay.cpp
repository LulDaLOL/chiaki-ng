// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "dadisplay.h"

#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
constexpr int kRampSize = 256;
constexpr double kMaxBoostGamma = 1.6; // boost 100 -> gamma 1.6 (brighter midtones, blacks stay black)
const char *kBoostKey = "da/display_boost";

#ifdef _WIN32
typedef HDC (WINAPI *GetDCFn)(HWND);
typedef int (WINAPI *ReleaseDCFn)(HWND, HDC);
typedef BOOL (WINAPI *GammaRampFn)(HDC, LPVOID);

struct GammaApi
{
	GetDCFn get_dc = nullptr;
	ReleaseDCFn release_dc = nullptr;
	GammaRampFn get_ramp = nullptr;
	GammaRampFn set_ramp = nullptr;
	bool ok() const { return get_dc && release_dc && get_ramp && set_ramp; }
};

GammaApi LoadGammaApi()
{
	GammaApi api;
	HMODULE user32 = LoadLibraryW(L"user32.dll");
	HMODULE gdi32 = LoadLibraryW(L"gdi32.dll");
	if(user32)
	{
		api.get_dc = reinterpret_cast<GetDCFn>(GetProcAddress(user32, "GetDC"));
		api.release_dc = reinterpret_cast<ReleaseDCFn>(GetProcAddress(user32, "ReleaseDC"));
	}
	if(gdi32)
	{
		api.get_ramp = reinterpret_cast<GammaRampFn>(GetProcAddress(gdi32, "GetDeviceGammaRamp"));
		api.set_ramp = reinterpret_cast<GammaRampFn>(GetProcAddress(gdi32, "SetDeviceGammaRamp"));
	}
	return api; // system DLLs stay loaded for the life of the process
}

const GammaApi &Gamma()
{
	static const GammaApi api = LoadGammaApi();
	return api;
}

void HideConsoleWindow(QProcess *process)
{
	process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
		args->flags |= CREATE_NO_WINDOW;
	});
}
#endif

void StartPowerShell(QProcess *process, const QString &command)
{
#ifdef _WIN32
	HideConsoleWindow(process);
#endif
	process->start(QStringLiteral("powershell.exe"),
		{ QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-Command"), command });
}
}

DaDisplay::DaDisplay(QObject *parent)
	: QObject(parent)
{
	QSettings settings;
	int stored = settings.value(kBoostKey, 0).toInt();
	boost_value = std::clamp(stored, 0, 100);
	if(boost_value > 0 && !applyBoost(boost_value))
		boost_value = 0;

	connect(qApp, &QCoreApplication::aboutToQuit, this, &DaDisplay::restoreDisplay);
	refresh();
}

DaDisplay::~DaDisplay()
{
	restoreDisplay();
}

void DaDisplay::refresh()
{
#ifdef _WIN32
	auto *process = new QProcess(this);
	connect(process, &QProcess::finished, this, [this, process](int exit_code, QProcess::ExitStatus status) {
		bool ok = false;
		int value = QString::fromLatin1(process->readAllStandardOutput()).trimmed().toInt(&ok);
		bool supported = status == QProcess::NormalExit && exit_code == 0 && ok && value >= 0 && value <= 100;
		if(supported && value != screen_brightness && !brightness_write_running)
		{
			screen_brightness = value;
			emit screenBrightnessChanged();
		}
		if(supported != screen_brightness_supported)
		{
			screen_brightness_supported = supported;
			emit screenBrightnessSupportedChanged();
		}
		process->deleteLater();
	});
	connect(process, &QProcess::errorOccurred, process, [process](QProcess::ProcessError error) {
		if(error == QProcess::FailedToStart) // no finished() in this case
			process->deleteLater();
	});
	StartPowerShell(process, QStringLiteral(
		"@(Get-CimInstance -Namespace root/WMI -ClassName WmiMonitorBrightness -ErrorAction Stop)[0].CurrentBrightness"));
#endif
}

void DaDisplay::setScreenBrightness(int value)
{
	value = std::clamp(value, 0, 100);
	if(value == screen_brightness)
		return;
	screen_brightness = value;
	emit screenBrightnessChanged();
	pending_brightness = value;
	if(!brightness_write_running)
		launchBrightnessWrite();
}

void DaDisplay::launchBrightnessWrite()
{
#ifdef _WIN32
	if(pending_brightness < 0)
		return;
	int target = pending_brightness;
	pending_brightness = -1;
	brightness_write_running = true;

	auto *process = new QProcess(this);
	auto done = [this, process]() {
		brightness_write_running = false;
		process->deleteLater();
		if(pending_brightness >= 0) // the slider moved again while Windows was applying the last value
			launchBrightnessWrite();
	};
	connect(process, &QProcess::finished, this, [done](int, QProcess::ExitStatus) { done(); });
	connect(process, &QProcess::errorOccurred, this, [done](QProcess::ProcessError error) {
		if(error == QProcess::FailedToStart) // no finished() in this case
			done();
	});
	StartPowerShell(process, QStringLiteral(
		"Get-CimInstance -Namespace root/WMI -ClassName WmiMonitorBrightnessMethods -ErrorAction Stop | "
		"Invoke-CimMethod -MethodName WmiSetBrightness -Arguments @{Timeout=[uint32]1; Brightness=[byte]%1} | Out-Null")
		.arg(target));
#endif
}

void DaDisplay::setBoost(int value)
{
	value = std::clamp(value, 0, 100);
	if(value == boost_value)
		return;
	bool ok = applyBoost(value);
	if(ok != boost_supported)
	{
		boost_supported = ok;
		emit boostSupportedChanged();
	}
	boost_value = ok ? value : 0;
	QSettings().setValue(kBoostKey, boost_value);
	emit boostChanged();
}

bool DaDisplay::applyBoost(int value)
{
#ifdef _WIN32
	const GammaApi &api = Gamma();
	if(!api.ok())
		return false;
	HDC dc = api.get_dc(nullptr);
	if(!dc)
		return false;

	bool ok = false;
	if(!have_saved_ramp)
	{
		saved_ramp.assign(3 * kRampSize, 0);
		have_saved_ramp = api.get_ramp(dc, saved_ramp.data()) != FALSE;
	}

	if(value <= 0)
	{
		ok = have_saved_ramp ? api.set_ramp(dc, saved_ramp.data()) != FALSE : true;
	}
	else
	{
		// Windows rejects ramps that stray too far from normal; step the gamma down until one is accepted.
		std::vector<uint16_t> ramp(3 * kRampSize);
		for(double gamma = 1.0 + (kMaxBoostGamma - 1.0) * value / 100.0; gamma > 1.0001 && !ok; gamma -= 0.05)
		{
			for(int i = 0; i < kRampSize; i++)
			{
				double v = std::pow(i / double(kRampSize - 1), 1.0 / gamma) * 65535.0;
				uint16_t level = static_cast<uint16_t>(std::clamp(v, 0.0, 65535.0));
				ramp[i] = level;
				ramp[kRampSize + i] = level;
				ramp[2 * kRampSize + i] = level;
			}
			ok = api.set_ramp(dc, ramp.data()) != FALSE;
		}
	}
	api.release_dc(nullptr, dc);
	return ok;
#else
	return value <= 0;
#endif
}

void DaDisplay::restoreDisplay()
{
#ifdef _WIN32
	if(!have_saved_ramp || boost_value <= 0)
		return;
	const GammaApi &api = Gamma();
	if(!api.ok())
		return;
	HDC dc = api.get_dc(nullptr);
	if(!dc)
		return;
	api.set_ramp(dc, saved_ramp.data());
	api.release_dc(nullptr, dc);
#endif
}
