// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007 Licq developers
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

#include "contactgroup.h"

#include "contactbar.h"
#include "contactuser.h"

using namespace LicqQtGui;

ContactGroup::ContactGroup(unsigned short id, QString name)
  : ContactItem(ContactListModel::GroupItem),
    myGroupId(id),
    myName(name),
    myEvents(0),
    myVisibleContacts(0)
{
  if (myGroupId != 0)
  {
    mySortKey.sprintf("%10i", myGroupId);
  }
  else
  {
    // Put "Other Users" last when sorting
    mySortKey = QString("9999999999");
  }

  for (int i = 0; i < 3; ++i)
    myBars[i] = new ContactBar(static_cast<ContactListModel::SubGroupType>(i), this);
}

ContactGroup::~ContactGroup()
{
  // Remove all user instances in this group
  while (!myUsers.isEmpty())
    delete myUsers.takeFirst();

  for (int i = 0; i < 3; ++i)
    delete myBars[i];
}

ContactItem* ContactGroup::item(int row) const
{
  if (row < 3)
    return myBars[row];
  else
    return myUsers.value(row - 3);
}

ContactUser* ContactGroup::user(ContactUserData* u) const
{
  foreach (ContactUser* instance, myUsers)
  {
    if (instance->userData() == u)
      return instance;
  }

  return 0;
}

int ContactGroup::rowCount() const
{
  // Add the separator bars
  return myUsers.size() + 3;
}

int ContactGroup::indexOf(ContactUser* user) const
{
  // The separator bars come first so add three to the index
  return myUsers.indexOf(user) + 3;
}

void ContactGroup::addUser(ContactUser* user, ContactListModel::SubGroupType subGroup)
{
  myUsers.append(user);
  if (user->visibility())
    myVisibleContacts++;
  myBars[subGroup]->countIncrease();
  myEvents += user->numEvents();
  myBars[subGroup]->updateNumEvents(user->numEvents());
  emit barDataChanged(myBars[subGroup], subGroup);

  emit dataChanged(this);
}

void ContactGroup::removeUser(ContactUser* user, ContactListModel::SubGroupType subGroup)
{
  myUsers.removeAll(user);
  if (user->visibility())
    myVisibleContacts--;
  myBars[subGroup]->countDecrease();
  myEvents -= user->numEvents();
  myBars[subGroup]->updateNumEvents(-user->numEvents());
  emit barDataChanged(myBars[subGroup], subGroup);

  emit dataChanged(this);
}

void ContactGroup::updateSubGroup(ContactListModel::SubGroupType oldSubGroup, ContactListModel::SubGroupType newSubGroup, int eventCounter)
{
  myBars[oldSubGroup]->countDecrease();
  myBars[oldSubGroup]->updateNumEvents(-eventCounter);
  emit barDataChanged(myBars[oldSubGroup], oldSubGroup);

  myBars[newSubGroup]->countIncrease();
  myBars[newSubGroup]->updateNumEvents(eventCounter);
  emit barDataChanged(myBars[newSubGroup], newSubGroup);
}

void ContactGroup::updateNumEvents(int counter, ContactListModel::SubGroupType subGroup)
{
  if (counter == 0)
    return;

  myEvents += counter;
  myBars[subGroup]->updateNumEvents(counter);

  emit dataChanged(this);
}

void ContactGroup::updateVisibility(bool increase)
{
  if (increase)
    myVisibleContacts++;
  else
    myVisibleContacts--;

  emit dataChanged(this);
}

QVariant ContactGroup::data(int column, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
      if (column == 0)
      {
        int onlineCount = myBars[ContactListModel::OnlineSubGroup]->count();
        if (onlineCount > 0)
          return myName + " (" + QString::number(onlineCount) + ")";
        else
          return myName;
      }
      break;

    case ContactListModel::ItemTypeRole:
      return ContactListModel::GroupItem;

    case ContactListModel::SortPrefixRole:
      return 0;

    case ContactListModel::SortRole:
      return mySortKey;

    case ContactListModel::UnreadEventsRole:
      return myEvents;

    case ContactListModel::GroupIdRole:
      return myGroupId;

    case ContactListModel::UserCountRole:
      return myUsers.size();

    case ContactListModel::VisibilityRole:
      return (myVisibleContacts > 0);
  }

  return QVariant();
}