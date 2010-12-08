/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "UtilsSettings.hpp"
#include "Protection.hpp"
#include "MainWindow.hpp"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "AirfieldDetails.h"
#include "Topology/TopologyStore.hpp"
#include "Topology/TopologyGlue.hpp"
#include "Dialogs/Dialogs.h"
#include "Device/device.hpp"
#include "Message.hpp"
#include "Polar/Loader.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Language.hpp"
#include "LogFile.hpp"
#include "Simulator.hpp"
#include "DrawThread.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "ProgressGlue.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "WayPoint/WayPointGlue.hpp"

#if defined(__BORLANDC__)  // due to compiler bug
  #include "Waypoint/Waypoints.hpp"
  #include "Polar/Polar.hpp"
#endif

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool TerrainFileChanged = false;
bool TopologyFileChanged = false;
bool PolarFileChanged = false;
bool LanguageFileChanged = false;
bool StatusFileChanged = false;
bool InputFileChanged = false;

static void
SettingsEnter()
{
#ifndef ENABLE_OPENGL
  draw_thread->suspend();
#endif

  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed (also prevents drawing)

  MapFileChanged = false;
  AirspaceFileChanged = false;
  AirfieldFileChanged = false;
  WaypointFileChanged = false;
  TerrainFileChanged = false;
  TopologyFileChanged = false;
  PolarFileChanged = false;
  LanguageFileChanged = false;
  StatusFileChanged = false;
  InputFileChanged = false;
  DevicePortChanged = false;
}

static void
SettingsLeave()
{
  if (!globalRunningEvent.test())
    return;

  XCSoarInterface::main_window.map.set_focus();

  SuspendAllThreads();

  if (MapFileChanged) {
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    WaypointFileChanged = true;
    TerrainFileChanged = true;
    TopologyFileChanged = true;
  }

  if ((WaypointFileChanged) || (TerrainFileChanged) || (AirfieldFileChanged)) {
    ProgressGlue::Create(_("Loading Terrain File..."));

    XCSoarInterface::main_window.map.set_terrain(NULL);

    // re-load terrain
    delete terrain;
    terrain = RasterTerrain::OpenTerrain(file_cache);

    // re-load waypoints
    WayPointGlue::ReadWaypoints(way_points, terrain);
    ReadAirfieldFile(way_points);

    // re-set home
    if (WaypointFileChanged || TerrainFileChanged) {
      WayPointGlue::SetHome(way_points, terrain,
                            XCSoarInterface::SetSettingsComputer(),
                            WaypointFileChanged, false);
    }

    if (terrain != NULL) {
      RasterTerrain::UnprotectedLease lease(*terrain);
      lease->SetViewCenter(XCSoarInterface::Basic().Location);
    }

    XCSoarInterface::main_window.map.set_terrain(terrain);
  }

  if (TopologyFileChanged) {
    XCSoarInterface::main_window.map.set_topology(NULL);
    topology->Reset();
    LoadConfiguredTopology(*topology);
    XCSoarInterface::main_window.map.set_topology(topology);
  }

  if (AirspaceFileChanged) {
    if (airspace_warnings != NULL)
      airspace_warnings->clear();
    airspace_database.clear();
    ReadAirspace(airspace_database, terrain,
                 XCSoarInterface::Basic().pressure);
  }

  if (PolarFileChanged && protected_task_manager != NULL) {
    GlidePolar gp = protected_task_manager->get_glide_polar();
    if (LoadPolarById(XCSoarInterface::SettingsComputer(), gp)) {
      protected_task_manager->set_glide_polar(gp);
    }
  }

  if (protected_task_manager != NULL) {
    ProtectedTaskManager::ExclusiveLease lease(*protected_task_manager);
    lease->set_contest(XCSoarInterface::SettingsComputer().contest);
  }

  if (AirfieldFileChanged
      || AirspaceFileChanged
      || WaypointFileChanged
      || TerrainFileChanged
      || TopologyFileChanged) {
    ProgressGlue::Close();
    XCSoarInterface::main_window.map.set_focus();
#ifndef ENABLE_OPENGL
    draw_thread->trigger_redraw();
#endif
  }

  if (DevicePortChanged)
    devRestart();

  ResumeAllThreads();
  // allow map and calculations threads to continue
}

void
SystemConfiguration()
{
  if (!is_simulator() &&
      XCSoarInterface::LockSettingsInFlight &&
      XCSoarInterface::Basic().flight.Flying) {
    Message::AddMessage(_("Settings locked in flight"));
    return;
  }

  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave();
}

