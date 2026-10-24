# Third-Party Notices

QSnapCraft uses the following third-party components in the Windows release package.

## Qt 6

This application uses Qt 6.7.2 modules including Core, Gui, Widgets and Network.
Qt is available under the GNU Lesser General Public License version 3 (LGPLv3).

- Project: https://www.qt.io/
- Source and licensing: https://www.qt.io/licensing/open-source-lgpl-obligations
- License text: `licenses/LGPL-3.0-only.txt` in the release package
- Additional terms: `licenses/GPL-3.0-only.txt` in the release package

The Qt libraries are shipped as separate dynamic libraries so they can be replaced or
relinked as required by the LGPL. No Qt source code is modified by QSnapCraft.

## Microsoft Visual C++ Runtime

The release package may include the Microsoft Visual C++ Redistributable runtime required
by the MSVC build. Redistribution is subject to the Microsoft Visual Studio license terms.

## Microsoft Windows APIs

QSnapCraft uses Windows system APIs for display capture, window discovery, OCR, global
hotkeys, diagnostics and desktop integration. These components are part of the operating
system and are not redistributed by QSnapCraft.
