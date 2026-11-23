# DA Remote Play

PS5 Remote Play for Windows, built on [chiaki-ng](https://github.com/chiaki-ng/chiaki-ng) (AGPL-3.0).

Changes from chiaki-ng:
- DA Remote Play name, icon and dark theme; redesigned home screen with console cards and quick
  Stream (720p/1080p) and Picture (native / upscaled to screen) controls
- 20 Mbps default bitrate for home streaming
- Stream boost on Windows while streaming: Wi-Fi background scans paused (fixes repeating lag
  spikes), 1 ms timer resolution, above-normal priority, no power throttling, PC kept awake

PS5 Remote Play streams at most 1080p; the upscaled Picture modes render it at your screen's resolution.

Build: Actions > "Build DA Remote Play Windows (MSYS2)" > Run workflow.

---


![chiaki-ng Logo](gui/res/chiaking-logo.svg)

# [chiaki-ng](https://chiaki-ng.github.io/chiaki-ng/)

An open source PlayStation remote play project serving as the next-generation of Chiaki with improvements and ongoing support now that the original Chiaki project is in maintenance mode only. [Click here to see the accompanying site for documentation, updates and more](https://chiaki-ng.github.io/chiaki-ng/).

## Discord
[chiaki-ng community Discord](https://discord.gg/tAMbRuwXDH)

## Official downloads
Only download chiaki-ng from these sources:

- [GitHub Releases](https://github.com/chiaki-ng/chiaki-ng/releases)
- [Flathub](https://flathub.org/apps/io.github.streetpea.Chiaki4deck)
- winget: `winget install --id=StreetPea.chiaki-ng -e`
- Homebrew: `brew install --cask streetpea/streetpea/chiaki-ng`

Other websites offering chiaki-ng downloads, such as chiaki-ng.com, are not run by this project, and we can't vouch for the files they serve.

## Disclaimer
This project is not endorsed or certified by Sony Interactive Entertainment LLC.

Chiaki is a Free and Open Source Software Client for PlayStation 4 and PlayStation 5 Remote Play
for Linux, FreeBSD, OpenBSD, Android, macOS, Windows, Nintendo Switch and potentially even more platforms.
