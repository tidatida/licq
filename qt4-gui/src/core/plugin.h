/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2011, 2013 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef LICQGUI_QTGUIPLUGIN_H
#define LICQGUI_QTGUIPLUGIN_H

#include "config.h"

#include <licq/plugin/generalpluginhelper.h>


namespace LicqQtGui
{

class QtGuiPlugin : public Licq::GeneralPluginHelper
{
public:
  QtGuiPlugin();

  // Make inherited members public so rest of GUI can use them directly
  using Licq::GeneralPluginHelper::getReadPipe;
  using Licq::GeneralPluginHelper::popEvent;
  using Licq::GeneralPluginHelper::popSignal;
  using Licq::GeneralPluginHelper::setSignalMask;

  // From Licq::PluginInterface
  std::string name() const;
  std::string version() const;
  bool init(int argc, char** argv);
  int run();
  void destructor();

  // From Licq::GeneralPluginInterface
  std::string description() const;
  std::string usage() const;
  std::string configFile() const;
  bool isEnabled() const;

private:
  int myArgc;
  char** myArgv;
};

extern QtGuiPlugin* gQtGuiPlugin;

} // namespace LicqQtGui

#endif
