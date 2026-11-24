// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

// DA Remote Play: brightness controls that affect everything on screen.
//  - screenBrightness: the laptop panel backlight (0-100), through Windows WMI.
//    Not available on most external monitors.
//  - boost: brightens midtones of the whole display with a gamma ramp (0 = off, 100 = max)
//    while DA Remote Play is open; the original ramp is restored on exit or when set to 0.

#ifndef DA_DISPLAY_H
#define DA_DISPLAY_H

#include <QObject>

#include <cstdint>
#include <vector>

class DaDisplay : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int screenBrightness READ screenBrightness WRITE setScreenBrightness NOTIFY screenBrightnessChanged)
	Q_PROPERTY(bool screenBrightnessSupported READ screenBrightnessSupported NOTIFY screenBrightnessSupportedChanged)
	Q_PROPERTY(int boost READ boost WRITE setBoost NOTIFY boostChanged)
	Q_PROPERTY(bool boostSupported READ boostSupported NOTIFY boostSupportedChanged)

public:
	explicit DaDisplay(QObject *parent = nullptr);
	~DaDisplay() override;

	int screenBrightness() const { return screen_brightness; }
	void setScreenBrightness(int value);
	bool screenBrightnessSupported() const { return screen_brightness_supported; }

	int boost() const { return boost_value; }
	void setBoost(int value);
	bool boostSupported() const { return boost_supported; }

	Q_INVOKABLE void refresh();
	Q_INVOKABLE void restoreDisplay();

signals:
	void screenBrightnessChanged();
	void screenBrightnessSupportedChanged();
	void boostChanged();
	void boostSupportedChanged();

private:
	void launchBrightnessWrite();
	bool applyBoost(int value);

	int screen_brightness = 100;
	bool screen_brightness_supported = false;
	int pending_brightness = -1;
	bool brightness_write_running = false;

	int boost_value = 0;
	bool boost_supported = true;
	bool have_saved_ramp = false;
	std::vector<uint16_t> saved_ramp;
};

#endif // DA_DISPLAY_H
