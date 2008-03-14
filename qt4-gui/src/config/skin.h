/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 1999-2006 Licq developers
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

// Skin Spec 0.1

#ifndef SKIN_H
#define SKIN_H

#include "config.h"

#include <QColor>
#include <QObject>
#include <QPalette>
#include <QPixmap>
#include <QRect>

class CIniFile;

namespace LicqQtGui
{
namespace Config
{
class Border
{
public:
  unsigned short top, bottom;
  unsigned short left, right;

  void AdjustForMenuBar(unsigned short h1, unsigned short h2);
};

class FrameSkin
{
public:
  struct Border border;
  unsigned short frameStyle;
  bool maintainBorder;
  bool hasMenuBar;
  bool transparent;
  QPixmap pixmap;
  QPixmap mask;

  virtual ~FrameSkin() {}
  virtual void loadSkin(CIniFile& skinFile, QString name, QString baseSkinDir);
};

class ShapeSkin
{
public:
  QRect rect;
  QColor foreground;
  QColor background;

  virtual ~ShapeSkin() { }
  virtual void loadSkin(CIniFile& skinFile, QString name);
  QRect borderToRect(const QWidget* w) const;
  void AdjustForMenuBar(unsigned short h1, unsigned short h2);
};

class ButtonSkin : public ShapeSkin
{
public:
  QPixmap pixmapUpFocus;
  QPixmap pixmapUpNoFocus;
  QPixmap pixmapDown;
  QString caption;

  virtual ~ButtonSkin() { }
  virtual void loadSkin(CIniFile& skinFile, QString name, QString baseSkinDir);

private:
  using ShapeSkin::loadSkin;
};

class LabelSkin : public ShapeSkin
{
public:
  QPixmap pixmap;
  unsigned short frameStyle;
  bool transparent;
  unsigned short margin;

  virtual ~LabelSkin() { }
  virtual void loadSkin(CIniFile& skinFile, QString name, QString baseSkinDir);

private:
  using ShapeSkin::loadSkin;
};

class ComboSkin : public ShapeSkin { };

class ListSkin : public ShapeSkin { };

/**
 * Data for a gui skin
 * A singleton instance is used for the skin currently used for the gui.
 * Other instances may also be created to hold data for other skins, used by
 * SkinBrowser dialog.
 */
class Skin : public QObject
{
  Q_OBJECT

public:
  /**
   * Create the active skin instance
   *
   * @param skinName Initial skin name to load
   * @param parent Parent object
   */
  static void createInstance(QString skinName = QString(), QObject* parent = NULL);

  /**
   * Get the active skin
   *
   * @return The skin data singleton
   */
  static Skin* active()
  { return myInstance; }


  Skin(QString skinName = QString(), QObject* parent = NULL);
  virtual ~Skin() {}
  void loadSkin(QString skinName);

  QString skinName() const { return mySkinName; }
  QPixmap mainwinPixmap(int width, int height) const;
  QPixmap mainwinMask(int width, int height) const;

  void setFrameTransparent(bool transparent);
  void setFrameStyle(unsigned short frameStyle);

  FrameSkin frame;
  ButtonSkin btnSys;
  LabelSkin lblStatus, lblMsg;
  ComboSkin cmbGroups;
  ListSkin lstUsers;

  QColor backgroundColor;
  QColor gridlineColor;
  QColor scrollbarColor;
  QColor buttonTextColor;

  QColor onlineColor;
  QColor offlineColor;
  QColor awayColor;
  QColor newUserColor;
  QColor awaitingAuthColor;

  QColor highBackColor;
  QColor highTextColor;

  QColor groupBackColor;
  QColor groupTextColor;
  QColor groupHighBackColor;
  QColor groupHighTextColor;

  QImage groupBackImage;

  bool tileGroupBackImage;

  // Functions
  void AdjustForMenuBar(unsigned short n);
  int frameWidth(void);
  int frameHeight(void);
  QPalette palette(QWidget* parent);

signals:
  void changed();
  void frameChanged();

private:
  // Singleton instance
  static Skin* myInstance;

  void SetDefaultValues();
  QPixmap scaleWithBorder(const QPixmap& pm, int width, int height) const;

  QString mySkinName;
  unsigned short myMenuBarHeight;
};

} // namespace Config
} // namespace LicqQtGui

#endif