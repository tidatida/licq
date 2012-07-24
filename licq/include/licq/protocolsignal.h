/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010-2012 Licq developers <licq-dev@googlegroups.com>
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

#ifndef LICQ_PROTOCOLSIGNAL_H
#define LICQ_PROTOCOLSIGNAL_H

#include <list>
#include <pthread.h>
#include <string>

#include "userid.h"

namespace Licq
{

class ProtocolSignal
{
public:
  enum SignalType
  {
    SignalLogon         = 1,    // Log on
    SignalLogoff        = 2,    // Log off
    SignalChangeStatus  = 3,    // Change status
    SignalAddUser       = 4,    // Add a contact to list
    SignalRemoveUser    = 5,    // Remove a contact from list
    SignalRenameUser    = 6,    // Rename a contact in server list
    SignalChangeUserGroups=7,   // Change groups for a user
    SignalSendMessage   = 8,    // Send a message to a user
    SignalNotifyTyping  = 9,    // Send typing notification
    SignalGrantAuth     = 10,   // Grant authorization to a user
    SignalRefuseAuth    = 11,   // Refuse authorization for a user
    SignalRequestInfo   = 12,   // Request user information
    SignalUpdateInfo    = 13,   // Update owner information
    SignalRequestPicture= 14,   // Request user picture
    SignalBlockUser     = 15,   // Add user to invisible/block list
    SignalUnblockUser   = 16,   // Remove user from inivisible/block list
    SignalAcceptUser    = 17,   // Add user to visible/accept list
    SignalUnacceptUser  = 18,   // Removed user from visible/accept list
    SignalIgnoreUser    = 19,   // Add user to ignore list
    SignalUnignoreUser  = 20,   // Remove user from ignore list
    SignalSendFile      = 21,   // Send file to user
    SignalSendChat      = 22,   // Send chat request
    SignalCancelEvent   = 23,   // Cancel an event (chat, secure, file, etc.)
    SignalSendReply     = 24,   // Accept/refuse file/chat request
    SignalOpenSecure    = 27,   // Request secure channel with user
    SignalCloseSecure   = 28,   // Close secure channel with user
    SignalRequestAuth   = 29,   // Request authorization from user
  };

  // Flags for send events
  enum SendFlags
  {
    SendToList          = 1,    // Flag a message as low prio (send to contact list)
    SendUrgent          = 2,    // Flag a message as urgent
    SendDirect          = 4,    // Don't send message via server (if supported by protocol)
    SendToMultiple      = 8,    // Message is sent to multiple recipients
  };

  ProtocolSignal(SignalType signal, const UserId& userId = UserId(), unsigned long eventId = 0)
    : mySignal(signal),
      myUserId(userId),
      myEventId(eventId),
      myCallerThread(pthread_self())
  { /* Empty */ }

  virtual ~ProtocolSignal()
  { /* Empty */ }

  //! The signal is being sent to the plugin.
  SignalType signal() const
  { return mySignal; }

  //! The user id that this signal is being used for.
  const UserId& userId() const
  { return myUserId; }

  /// Id to use for event generated by this signal
  unsigned long eventId() const
  { return myEventId; }

  //! The calling thread.
  pthread_t callerThread() const
  { return myCallerThread; }


private:
  SignalType mySignal;
  UserId myUserId;
  unsigned long myEventId;
  pthread_t myCallerThread;
};


class ProtoLogonSignal : public ProtocolSignal
{
public:
  ProtoLogonSignal(unsigned status)
    : ProtocolSignal(SignalLogon),
      myStatus(status)
  { /* Empty */ }

  //! The requested initial status.
  unsigned status() const { return myStatus; }

private:
  unsigned myStatus;
};

class ProtoLogoffSignal : public ProtocolSignal
{
public:
  ProtoLogoffSignal()
    : ProtocolSignal(SignalLogoff)
  { /* Empty */ }
};

class ProtoChangeStatusSignal : public ProtocolSignal
{
public:
  ProtoChangeStatusSignal(unsigned status)
    : ProtocolSignal(SignalChangeStatus),
      myStatus(status)
  { /* Empty */ }

  //! The requested status.
  unsigned status() const { return myStatus; }

private:
  unsigned myStatus;
};

class ProtoAddUserSignal : public ProtocolSignal
{
public:
  ProtoAddUserSignal(const UserId& userId, bool authRequired)
    : ProtocolSignal(SignalAddUser, userId),
      myAuthRequired(authRequired)
  { /* Empty */ }

  //! True if authorization is required to add this user.
  bool authRequired() const { return myAuthRequired; }

private:
  bool myAuthRequired;
};

class ProtoRemoveUserSignal : public ProtocolSignal
{
public:
  ProtoRemoveUserSignal(const UserId& userId)
    : ProtocolSignal(SignalRemoveUser, userId)
  { /* Empty */ }
};

class ProtoRenameUserSignal : public ProtocolSignal
{
public:
  ProtoRenameUserSignal(const UserId& userId)
    : ProtocolSignal(SignalRenameUser, userId)
  { /* Empty */ }
};

class ProtoChangeUserGroupsSignal : public ProtocolSignal
{
public:
  ProtoChangeUserGroupsSignal(const UserId& userId, int groupId)
    : ProtocolSignal(SignalChangeUserGroups, userId),
      myGroupId(groupId)
  { /* Empty */ }

  int groupId() const { return myGroupId; }
private:
  int myGroupId;
};

class ProtoSendMessageSignal : public ProtocolSignal
{
public:
  ProtoSendMessageSignal(unsigned long eventId, const UserId& userId,
      const std::string& message, unsigned flags,
      unsigned long convoId = 0)
    : ProtocolSignal(SignalSendMessage, userId, eventId),
      myMessage(message),
      myFlags(flags),
      myConvoId(convoId)
  { /* Empty */ }

  //! The message to be sent
  const std::string& message() const { return myMessage; }
  //! The message flags
  unsigned flags() const { return myFlags; }
  //! The conversation id to use (gets the socket).
  unsigned long convoId() const { return myConvoId; }

private:
  std::string myMessage;
  unsigned myFlags;
  unsigned long myConvoId;
};

class ProtoTypingNotificationSignal : public ProtocolSignal
{
public:
  ProtoTypingNotificationSignal(const UserId& userId, bool active, unsigned long convoId = 0)
    : ProtocolSignal(SignalNotifyTyping, userId),
      myActive(active),
      myConvoId(convoId)
  { /* Empty */ }

  bool active() const { return myActive; }
  //! The conversation id to use (gets the socket).
  unsigned long convoId() const { return myConvoId; }

private:
  bool myActive;
  unsigned long myConvoId;
};

class ProtoGrantAuthSignal : public ProtocolSignal
{
public:
  ProtoGrantAuthSignal(unsigned long eventId, const UserId& userId, const std::string& message)
    : ProtocolSignal(SignalGrantAuth, userId, eventId),
      myMessage(message)
  { /* Empty */ }

  const std::string& message() const { return myMessage; }

private:
  std::string myMessage;
};

class ProtoRefuseAuthSignal : public ProtocolSignal
{
public:
  ProtoRefuseAuthSignal(unsigned long eventId, const UserId& userId, const std::string& message)
    : ProtocolSignal(SignalRefuseAuth, userId, eventId),
      myMessage(message)
  { /* Empty */ }

  const std::string& message() const { return myMessage; }

private:
  std::string myMessage;
};

class ProtoRequestInfo : public ProtocolSignal
{
public:
  ProtoRequestInfo(unsigned long eventId, const UserId& userId)
    : ProtocolSignal(SignalRequestInfo, userId, eventId)
  { /* Empty */ }
};

class ProtoUpdateInfoSignal : public ProtocolSignal
{
public:
  ProtoUpdateInfoSignal(unsigned long eventId, const UserId& ownerId)
    : ProtocolSignal(SignalUpdateInfo, ownerId, eventId)
  { /* Empty */ }
};

class ProtoRequestPicture : public ProtocolSignal
{
public:
  ProtoRequestPicture(unsigned long eventId, const UserId& userId)
    : ProtocolSignal(SignalRequestPicture, userId, eventId)
  { /* Empty */ }
};

class ProtoBlockUserSignal : public ProtocolSignal
{
public:
  ProtoBlockUserSignal(const UserId& userId)
    : ProtocolSignal(SignalBlockUser, userId)
  { /* Empty */ }
};

class ProtoUnblockUserSignal : public ProtocolSignal
{
public:
  ProtoUnblockUserSignal(const UserId& userId)
    : ProtocolSignal(SignalUnblockUser, userId)
  { /* Empty */ }
};

class ProtoAcceptUserSignal : public ProtocolSignal
{
public:
  ProtoAcceptUserSignal(const UserId& userId)
    : ProtocolSignal(SignalAcceptUser, userId)
  { /* Empty */ }
};

class ProtoUnacceptUserSignal : public ProtocolSignal
{
public:
  ProtoUnacceptUserSignal(const UserId& userId)
    : ProtocolSignal(SignalUnacceptUser, userId)
  { /* Empty */ }
};

class ProtoIgnoreUserSignal : public ProtocolSignal
{
public:
  ProtoIgnoreUserSignal(const UserId& userId)
    : ProtocolSignal(SignalIgnoreUser, userId)
  { /* Empty */ }
};

class ProtoUnignoreUserSignal : public ProtocolSignal
{
public:
  ProtoUnignoreUserSignal(const UserId& userId)
    : ProtocolSignal(SignalUnignoreUser, userId)
  { /* Empty */ }
};

class ProtoSendFileSignal : public ProtocolSignal
{
public:
  ProtoSendFileSignal(unsigned long eventId, const UserId& userId, const std::string& filename,
      const std::string& message, const std::list<std::string>& files)
    : ProtocolSignal(SignalSendFile, userId, eventId),
      myFilename(filename),
      myMessage(message),
      myFiles(files)
  { /* Empty */ }

  const std::string& filename() const { return myFilename; }
  const std::string& message() const { return myMessage; }
  const std::list<std::string>& files() const { return myFiles; }

private:
  std::string myFilename;
  std::string myMessage;
  std::list<std::string> myFiles;
};

class ProtoSendChatSignal : public ProtocolSignal
{
public:
  ProtoSendChatSignal(const UserId& userId, const std::string& message)
    : ProtocolSignal(SignalSendChat, userId),
      myMessage(message)
  { /* Empty */ }

  const std::string& message() const { return myMessage; }

private:
  std::string myMessage;
};

class ProtoCancelEventSignal : public ProtocolSignal
{
public:
  ProtoCancelEventSignal(const UserId& userId, unsigned long flag)
    : ProtocolSignal(SignalCancelEvent, userId),
      myFlag(flag)
  { /* Empty */ }

  unsigned long flag() const { return myFlag; }

private:
  unsigned long myFlag;
};

class ProtoSendEventReplySignal : public ProtocolSignal
{
public:
  ProtoSendEventReplySignal(const UserId& userId, const std::string& message,
      bool accepted, unsigned short port, unsigned long sequence = 0,
      unsigned long flag1 = 0, unsigned long flag2 = 0, bool direct = false)
    : ProtocolSignal(SignalSendReply, userId),
      myMessage(message),
      myAccept(accepted),
      myPort(port),
      mySequence(sequence),
      myFlag1(flag1),
      myFlag2(flag2),
      myDirect(direct)
  { /* Empty */ }

  const std::string& message() const { return myMessage; }
  bool accept() const { return myAccept; }
  unsigned short port() const { return myPort; }
  unsigned long sequence() const { return mySequence; }
  unsigned long flag1() const { return myFlag1; }
  unsigned long flag2() const { return myFlag2; }
  bool direct() const { return myDirect; }

private:
  std::string myMessage;
  bool myAccept;
  unsigned short myPort;
  unsigned long mySequence;
  unsigned long myFlag1;
  unsigned long myFlag2;
  bool myDirect;
};

class ProtoOpenSecureSignal : public ProtocolSignal
{
public:
  ProtoOpenSecureSignal(unsigned long eventId, const UserId& userId)
    : ProtocolSignal(SignalOpenSecure, userId, eventId)
  { /* Empty */ }
};

class ProtoCloseSecureSignal : public ProtocolSignal
{
public:
  ProtoCloseSecureSignal(unsigned long eventId, const UserId& userId)
    : ProtocolSignal(SignalCloseSecure, userId, eventId)
  { /* Empty */ }
};

class ProtoRequestAuthSignal : public ProtocolSignal
{
public:
  ProtoRequestAuthSignal(const UserId& userId, const std::string& message)
    : ProtocolSignal(SignalRequestAuth, userId),
      myMessage(message)
  { /* Empty */ }

  const std::string& message() const { return myMessage; }

private:
  std::string myMessage;
};

} // namespace Licq

#endif
