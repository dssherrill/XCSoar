/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_KOBO_WIFI_HPP
#define XCSOAR_KOBO_WIFI_HPP

#include "Util/StaticString.hpp"
#include "OS/SocketDescriptor.hpp"

struct WifiStatus {
  StaticString<32> bssid;
  StaticString<256> ssid;

  void Clear() {
    bssid.clear();
    ssid.clear();
  }
};

struct WifiNetworkInfo {
  StaticString<32> bssid;
  StaticString<256> ssid;
};

class WPASupplicant {
  SocketDescriptor fd;

  char local_path[32];

public:
  ~WPASupplicant() {
    Close();
  }

  gcc_pure
  bool IsConnected() const {
    // TODO: what if the socket is broken?
    return fd.IsDefined();
  }

  bool Connect(const char *path);
  void Close();

  bool SendCommand(const char *cmd);
  bool ExpectResponse(const char *expected);

  bool Status(WifiStatus &status);

  bool Scan();

  /**
   * @return the number of networks or -1 on error
   */
  int ScanResults(WifiNetworkInfo *dest, unsigned max);
};

#endif