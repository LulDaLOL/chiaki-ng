// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

// DA Remote Play: while a stream runs on Windows,
//  - pause Wi-Fi background scans and turn on WLAN media streaming mode
//    (background scans cause lag spikes that repeat every 30-60 seconds on many laptops),
//  - raise the Windows timer resolution to 1 ms (steadier network and frame pacing),
//  - run the app at above-normal priority and opt it out of power throttling,
//  - keep the PC and display awake.
// Everything is restored when the stream stops.

#ifndef DA_STREAMBOOST_H
#define DA_STREAMBOOST_H

#ifdef _WIN32
void DaStreamBoostStart();
void DaStreamBoostStop();
#else
inline void DaStreamBoostStart() {}
inline void DaStreamBoostStop() {}
#endif

#endif // DA_STREAMBOOST_H
