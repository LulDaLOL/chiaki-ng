// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

// DA Remote Play: while a stream runs, pause Windows Wi-Fi background scans and turn on
// WLAN media streaming mode. Background scans cause the lag spikes that repeat every
// 30-60 seconds on many laptops. Everything is restored when the stream stops.

#ifndef DA_WIFIBOOST_H
#define DA_WIFIBOOST_H

#ifdef _WIN32
void DaWifiBoostStart();
void DaWifiBoostStop();
#else
inline void DaWifiBoostStart() {}
inline void DaWifiBoostStop() {}
#endif

#endif // DA_WIFIBOOST_H
