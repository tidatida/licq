/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007-2009 Licq developers
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

#ifndef SKINNABLECOMBOBOX_H
#define SKINNABLECOMBOBOX_H

#include <QComboBox>

namespace LicqQtGui
{

namespace Config
{
class ComboSkin;
}

/**
 * Extended QComboBox which can be skinned
 */
class SkinnableComboBox : public QComboBox
{
  Q_OBJECT

public:
  /**
   * Constructor, create a skinnable combobox and apply a skin
   *
   * @param skin Combobox skin to apply
   * @param parent Parent widget
   */
  SkinnableComboBox(const Config::ComboSkin& skin, QWidget* parent = NULL);

  /**
   * Constructor, create a default skinnable combobox
   *
   * @param parent Parent widget
   */
  SkinnableComboBox(QWidget* parent = 0);

  /**
   * Apply a skin
   *
   * @param skin New skin to use
   */
  void applySkin(const Config::ComboSkin& skin);
};

} // namespace LicqQtGui

#endif
