/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// written by Jon Keating <jon@licq.org>

#include "msn.h"
#include "msnpacket.h"
#include "licq_log.h"
#include "licq_message.h"

#include <unistd.h>

#include <cassert>
#include <string>
#include <list>
#include <vector>

#include <licq/md5.h>
#include <licq/oneventmanager.h>
#include <licq_user.h>

using namespace std;
using Licq::OnEventManager;
using Licq::gOnEventManager;


void CMSN::ProcessServerPacket(CMSNBuffer *packet)
{
  char szCommand[4];
  CMSNPacket *pReply;
  
//while (!m_pPacketBuf->End())
  {
    pReply = 0;
    packet->UnpackRaw(szCommand, 3);
    string strCmd(szCommand);
    
    if (strCmd == "VER")
    {
      // Don't really care about this packet's data.
      pReply = new CPS_MSNClientVersion(m_szUserName);
    }
    else if (strCmd == "CVR")
    {
      // Don't really care about this packet's data.
      pReply = new CPS_MSNUser(m_szUserName);
    }
    else if (strCmd == "XFR")
    {
      //Time to transfer to a new server
      packet->SkipParameter(); // Seq
      string strServType = packet->GetParameter();
      string strServer = packet->GetParameter();
    
      if (strServType == "SB")
      {
        packet->SkipParameter(); // 'CKI'
        string strCookie = packet->GetParameter();
        
        MSNSBConnectStart(strServer, strCookie);
      }
      else
      {
        size_t sep = strServer.rfind(':');
        string host;
        int port = 0;
        if (sep != string::npos)
        {
          host = strServer.substr(0, sep);
          port = atoi(strServer.substr(sep+1).c_str());
        }

        gSocketMan.CloseSocket(m_nServerSocket, false, true);
  
        // Make the new connection
        MSNLogon(host.c_str(), port, m_nStatus);
      }
    }
    else if (strCmd == "USR")
    {
      packet->SkipParameter(); // Seq
      string strType = packet->GetParameter();
      
      if (strType == "OK")
      {
        packet->SkipParameter(); // email account
        string strNick = packet->GetParameter();
        string strDecodedNick = Decode(strNick);
        gLog.Info("%s%s logged in.\n", L_MSNxSTR, strDecodedNick.c_str());
       
        // Set our alias here
        ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);
        o->setAlias(strDecodedNick);
        gUserManager.DropOwner(o);
         
        // This cookie doesn't work anymore now that we are online
        if (m_szCookie)
        {
          free(m_szCookie);
          m_szCookie = 0;
        }

        pReply = new CPS_MSNSync(m_nListVersion);
      }
      else
      {
        packet->SkipParameter(); // "S"
        string strParam = packet->GetParameter();
      
        m_szCookie = strdup(strParam.c_str());

        //MSNGetServer();
        // Make an SSL connection to authenticate
        MSNAuthenticate(m_szCookie);
      }
    }
    else if (strCmd == "CHL")
    {
      packet->SkipParameter(); // Seq
      string strHash = packet->GetParameter();
      
      pReply = new CPS_MSNChallenge(strHash.c_str());
    }
    else if (strCmd == "SYN")
    {
      packet->SkipParameter();
      string strVersion = packet->GetParameter();
      m_nListVersion = atol(strVersion.c_str());

      MSNChangeStatus(m_nStatus);

      // Send our local list now
      //FOR_EACH_PROTO_USER_START(MSN_PPID, LOCK_R)
      //{
      //  pReply = new CPS_MSNAddUser(pUser->IdString());
      //  SendPacket(pReply);
      //}
      //FOR_EACH_PROTO_USER_END
    }
    else if (strCmd == "LST")
    {
      // Add user
      string strUser = packet->GetParameter();
      string strNick = packet->GetParameter();
      string strLists = packet->GetParameter();
      string strUserLists;

      if (strUser == gUserManager.OwnerId(MSN_PPID))
        return;

      int nLists = atoi(strLists.c_str());
      if (nLists & FLAG_CONTACT_LIST)
        strUserLists = packet->GetParameter();

      UserId userId = LicqUser::makeUserId(strUser, MSN_PPID);
      if (nLists & FLAG_CONTACT_LIST)
        gUserManager.addUser(userId, true, false);

      LicqUser* u = gUserManager.fetchUser(userId, LOCK_W);
      if (u)
      {
        u->SetEnableSave(false);
        u->SetUserEncoding("UTF-8"); 
        u->SetInvisibleList(nLists & FLAG_BLOCK_LIST);
        
        if (!u->KeepAliasOnUpdate())
        {
          string strDecodedNick = Decode(strNick);
          u->setAlias(strDecodedNick);
          gLicqDaemon->pushPluginSignal(new LicqSignal(SIGNAL_UPDATExUSER, USER_BASIC, u->id()));
        }
        u->setUserInfoString("Email1", strUser);
        string strURL = "http://members.msn.com/"+strUser;
        u->setUserInfoString("Homepage", strURL);
        u->SetNewUser(false);
        u->SetEnableSave(true);
        u->SaveLicqInfo();
        gLicqDaemon->pushPluginSignal(new LicqSignal(SIGNAL_UPDATExUSER, USER_INFO, u->id()));
        gUserManager.DropUser(u);
      }
    }
    else if (strCmd == "LSG")
    {
      // Add group
    }
    else if (strCmd == "ADD")
    {
      packet->SkipParameter(); // What's this?
      string strList = packet->GetParameter();
      string strVersion = packet->GetParameter();
      string strUser = packet->GetParameter();
      UserId userId = LicqUser::makeUserId(strUser, MSN_PPID);
      string strNick = packet->GetParameter();
      m_nListVersion = atol(strVersion.c_str());
      
      if (strList == "RL")
      {
        gLog.Info("%sAuthorization request from %s.\n", L_MSNxSTR, strUser.c_str());

        CUserEvent* e = new CEventAuthRequest(userId,
          strNick.c_str(), "", "", "", "", ICQ_CMDxRCV_SYSxMSGxONLINE, time(0), 0);
      
        ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);
        if (gLicqDaemon->AddUserEvent(o, e))
        {
          e->AddToHistory(o, D_RECEIVER);
          gOnEventManager.performOnEvent(OnEventManager::OnEventSysMsg, o);
          gUserManager.DropOwner(o);
        }
        else
          gUserManager.DropOwner(o);
      }
      else
      {
        gLog.Info("%sAdded %s to contact list.\n", L_MSNxSTR, strUser.c_str());

        LicqUser* u = gUserManager.fetchUser(userId, LOCK_W);
        if (u)
        {
          if (!u->KeepAliasOnUpdate())
          {
            string strDecodedNick = Decode(strNick);
            u->setAlias(strDecodedNick);
          }
          gLicqDaemon->pushPluginSignal(new LicqSignal(SIGNAL_UPDATExUSER, USER_BASIC, u->id()));
          gUserManager.DropUser(u);
        }
      }
    }
    else if (strCmd == "REM")
    {      
      packet->SkipParameter(); // seq
      packet->SkipParameter(); // list
      string strVersion = packet->GetParameter();
      string strUser = packet->GetParameter();
      m_nListVersion = atol(strVersion.c_str());
    
      gLog.Info("%sRemoved %s from contact list.\n", L_MSNxSTR, strUser.c_str()); 
    }
    else if (strCmd == "REA")
    {
      packet->SkipParameter(); // seq
      string strVersion = packet->GetParameter();
      string strUser = packet->GetParameter();
      string strNick = packet->GetParameter();
      
      m_nListVersion = atol(strVersion.c_str());
      if (strcmp(m_szUserName, strUser.c_str()) == 0)
      {
        ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);
        string strDecodedNick = Decode(strNick);
        o->setAlias(strDecodedNick);
        gUserManager.DropOwner(o);
      }
      
      gLog.Info("%s%s renamed successfully.\n", L_MSNxSTR, strUser.c_str());
    }
    else if (strCmd == "CHG")
    {
      packet->SkipParameter(); // seq
      string strStatus = packet->GetParameter();
      ICQOwner* o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);
      unsigned long nStatus;
      bool bHidden = false;
      
      if (strStatus == "NLN")
        nStatus = ICQ_STATUS_ONLINE;
      else if (strStatus == "BSY")
        nStatus = ICQ_STATUS_OCCUPIED;
      else if (strStatus == "HDN")
      {
        nStatus = ICQ_STATUS_ONLINE | ICQ_STATUS_FxPRIVATE;
        bHidden = true;
      }
      else
        nStatus = ICQ_STATUS_AWAY;
        
      gLicqDaemon->ChangeUserStatus(o, nStatus);
      m_nStatus = nStatus;
      gLog.Info("%sServer says we are now: %s\n", L_MSNxSTR, ICQUser::StatusToStatusStr(o->Status(), bHidden));
      gUserManager.DropOwner(o);
    }
    else if (strCmd == "ILN" || strCmd == "NLN")
    {
      if (strCmd == "ILN")
        packet->SkipParameter(); // seq
      string strStatus = packet->GetParameter();
      string strUser = packet->GetParameter();
      string strNick = packet->GetParameter();
      string strClientId = packet->GetParameter();
      string strMSNObject = packet->GetParameter();
      string strDecodedObject = strMSNObject.size() ? Decode(strMSNObject) :"";

      unsigned short nStatus = ICQ_STATUS_AWAY;

      if (strStatus == "NLN")
        nStatus = ICQ_STATUS_ONLINE;
      else if(strStatus == "BSY")
        nStatus = ICQ_STATUS_OCCUPIED;

      ICQUser *u = gUserManager.FetchUser(strUser.c_str(), MSN_PPID, LOCK_W);
      if (u)
      {
        u->SetOnlineSince(time(NULL)); // Not in this protocol
        u->SetSendServer(true); // no direct connections

        if (!u->KeepAliasOnUpdate())
        {
          string strDecodedNick = Decode(strNick);
          u->setAlias(strDecodedNick);
          gLicqDaemon->pushPluginSignal(new LicqSignal(SIGNAL_UPDATExUSER, USER_BASIC, u->id()));
        }

	// Get the display picture here, so it can be shown with the notify
	if (strDecodedObject != u->GetPPField("MSNObject_DP"))
	{
	  u->SetPPField("MSNObject_DP", strDecodedObject);
	  if (strDecodedObject.size())
	  {
	    MSNGetDisplayPicture(u->IdString(), strDecodedObject);
	  }
	}

        gLog.Info("%s%s changed status (%s).\n", L_MSNxSTR, u->getAlias().c_str(), strStatus.c_str());
        gLicqDaemon->ChangeUserStatus(u, nStatus);

        if (strCmd == "NLN" && nStatus == ICQ_STATUS_ONLINE)
          gOnEventManager.performOnEvent(OnEventManager::OnEventOnline, u);
      }
      gUserManager.DropUser(u);
    }
    else if (strCmd == "FLN")
    {
      UserId userId = LicqUser::makeUserId(packet->GetParameter(), MSN_PPID);

      {
        LicqUserWriteGuard u(userId);
        if (u.isLocked())
        {
          gLog.Info("%s%s logged off.\n", L_MSNxSTR, u->getAlias().c_str());
          gLicqDaemon->ChangeUserStatus(*u, ICQ_STATUS_OFFLINE);
        }
      }

      // Do we have a connection attempt to this user?
      StartList::iterator it;
      pthread_mutex_lock(&mutex_StartList);
      for (it = m_lStart.begin(); it != m_lStart.end(); it++)
      {
        if (*it && userId == (*it)->userId)
        {
          gLog.Info("%sRemoving connection attempt to %s.\n", L_MSNxSTR, USERID_TOSTR(userId));
//          SStartMessage *pStart = (*it);
          m_lStart.erase(it);
          break;
        }
      }
      pthread_mutex_unlock(&mutex_StartList);
    }
    else if (strCmd == "RNG")
    {
      string strSessionID = packet->GetParameter();
      string strServer = packet->GetParameter();
      packet->SkipParameter(); // 'CKI'
      string strCookie = packet->GetParameter();
      string strUser = packet->GetParameter();
      
      MSNSBConnectAnswer(strServer, strSessionID, strCookie, strUser);
    }
    else if (strCmd == "MSG")
    {
      packet->SkipParameter(); // 'Hotmail'
      packet->SkipParameter(); // 'Hotmail' again
      packet->SkipParameter(); // size
      packet->SkipRN(); // Skip \r\n
      packet->ParseHeaders();
      
      string strType = packet->GetValue("Content-Type");
      
      if (strType.find("text/x-msmsgsprofile") != string::npos)
      {
        m_strMSPAuth = packet->GetValue("MSPAuth");
        m_strSID = packet->GetValue("sid");
        m_strKV = packet->GetValue("kv");
        m_nSessionStart = time(0);

        // We might have another packet attached
        //packet->SkipRN();
      }
      else if (strType.find("text/x-msmsgsinitialemailnotification") != string::npos)
      {
        // Email alert when we sign in
        
        // Get the next part..
        packet->SkipRN();
        packet->ParseHeaders();
      }
      else if (strType.find("text/x-msmsgsemailnotification") != string::npos)
      {
        // Email we get while signed in
        
        // Get the next part..
        packet->SkipRN();
        packet->ParseHeaders();
        
        string strFrom = packet->GetValue("From");
        string strFromAddr = packet->GetValue("From-Addr");
        string strSubject = packet->GetValue("Subject");
        
        string strToHash = m_strMSPAuth + "9" + m_szPassword;
        unsigned char szDigest[16];
        char szHexOut[32];
        Licq::md5((const uint8_t*)strToHash.c_str(), strToHash.size(), szDigest);
        for (int i = 0; i < 16; i++)
          sprintf(&szHexOut[i*2], "%02x", szDigest[i]);
    
        gLog.Info("%sNew email from %s (%s)\n", L_MSNxSTR, strFrom.c_str(), strFromAddr.c_str());
        CEventEmailAlert *pEmailAlert = new CEventEmailAlert(strFrom.c_str(), m_szUserName,
          strFromAddr.c_str(), strSubject.c_str(), time(0), m_strMSPAuth.c_str(), m_strSID.c_str(),
          m_strKV.c_str(), packet->GetValue("id").c_str(),
          packet->GetValue("Post-URL").c_str(), packet->GetValue("Message-URL").c_str(),
          szHexOut, m_nSessionStart);
          
        ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);
        if (gLicqDaemon->AddUserEvent(o, pEmailAlert))
        {
          pEmailAlert->AddToHistory(o, D_RECEIVER);
          gOnEventManager.performOnEvent(OnEventManager::OnEventSysMsg, o);
          gUserManager.DropOwner(o);
        }
        else
          gUserManager.DropOwner(o);
      }
    }
    else if (strCmd == "QNG")
    {
      m_bWaitingPingReply = false;
    }
    else if (strCmd == "913")
    {
      unsigned long nSeq = packet->GetParameterUnsignedLong();

      // Search pStart for this sequence, mark it as an error, send the
      // signals to the daemon and remove these item from the list.
      SStartMessage *pStart = 0;
      StartList::iterator it;
      pthread_mutex_lock(&mutex_StartList);
      for (it = m_lStart.begin(); it != m_lStart.end(); it++)
      {
        if ((*it)->m_nSeq == nSeq)
        {
          gLog.Error("%sCannot send messages while invisible.\n", L_ERRORxSTR);
          pStart = *it;
          pStart->m_pEvent->m_eResult = EVENT_FAILED;
          gLicqDaemon->PushPluginEvent(pStart->m_pEvent);
          m_lStart.erase(it);
          break; 
        }
      }     
      pthread_mutex_unlock(&mutex_StartList);
    }
    else if (strCmd == "GTC")
    {
    }
    else if (strCmd == "BLP")
    {
    }
    else if (strCmd == "PRP")
    {
    }    
    else if (strCmd == "QRY")
    {
      m_bCanPing = true;
    }
    else if (strCmd == "NOT")
    {
      // For the moment, skip the notification... consider it spam from MSN
      unsigned long nSize = packet->GetParameterUnsignedLong(); // size
      packet->SkipRN(); // Skip \r\n
      packet->Skip(nSize);
    }
    else
    {
      gLog.Warn("%sUnhandled command (%s).\n", L_MSNxSTR, strCmd.c_str());
    }
    
    if (pReply)
      SendPacket(pReply);
  }
}

void CMSN::SendPacket(CMSNPacket *p)
{
  INetSocket *s = gSocketMan.FetchSocket(m_nServerSocket);
  SrvSocket *sock = static_cast<SrvSocket *>(s);
  assert(sock != NULL);
  if (!sock->SendRaw(p->getBuffer()))
    MSNLogoff(true);
  else
    gSocketMan.DropSocket(sock);
  
  delete p;
}

void CMSN::MSNLogon(const char *_szServer, int _nPort)
{
  MSNLogon(_szServer, _nPort, m_nOldStatus);
}

void CMSN::MSNLogon(const char *_szServer, int _nPort, unsigned long _nStatus)
{
  if (_nStatus == ICQ_STATUS_OFFLINE)
    return;

  const ICQOwner* o = gUserManager.FetchOwner(MSN_PPID, LOCK_R);
  if (!o)
  {
    gLog.Error("%sNo owner set.\n", L_MSNxSTR);
    return;
  }
  m_szUserName = strdup(o->IdString());
  UserId myOwnerId = o->id();
  m_szPassword = strdup(o->Password());
  gUserManager.DropOwner(o);

  SrvSocket* sock = new SrvSocket(myOwnerId);
  gLog.Info("%sServer found at %s:%d.\n", L_MSNxSTR,
      _szServer, _nPort);

  if (!sock->connectTo(_szServer, _nPort))
  {
    gLog.Info("%sConnect failed to %s.\n", L_MSNxSTR, _szServer);
    delete sock;
    return;
  }
  
  gSocketMan.AddSocket(sock);
  m_nServerSocket = sock->Descriptor();
  gSocketMan.DropSocket(sock);
  
  CMSNPacket *pHello = new CPS_MSNVersion();
  SendPacket(pHello);
  m_nStatus = _nStatus;
}

void CMSN::MSNChangeStatus(unsigned long status)
{
  string msnStatus;
  if ((status & ICQ_STATUS_FxPRIVATE) != 0)
  {
    msnStatus = "HDN";
    status = ICQ_STATUS_ONLINE | ICQ_STATUS_FxPRIVATE;
  }
  else
  {
    switch (status & ~ICQ_STATUS_FxFLAGS)
    {
      case ICQ_STATUS_ONLINE:
      case ICQ_STATUS_FREEFORCHAT:
        msnStatus = "NLN";
        status = (status & ICQ_STATUS_FxFLAGS) | ICQ_STATUS_ONLINE;
        break;

      case ICQ_STATUS_OCCUPIED:
      case ICQ_STATUS_DND:
        msnStatus = "BSY";
        status = (status & ICQ_STATUS_FxFLAGS) | ICQ_STATUS_OCCUPIED;
        break;

      default:
        msnStatus = "AWY";
        status = (status & ICQ_STATUS_FxFLAGS) | ICQ_STATUS_AWAY;
        break;
    }
  }

  CMSNPacket* pSend = new CPS_MSNChangeStatus(msnStatus);
  SendPacket(pSend);
  m_nStatus = status;
}

void CMSN::MSNLogoff(bool bDisconnected)
{
  if (m_nServerSocket == -1) return;

  if (!bDisconnected)
  {
    CMSNPacket *pSend = new CPS_MSNLogoff();
    SendPacket(pSend);
  }
  
  m_nOldStatus = m_nStatus;
  m_nStatus = ICQ_STATUS_OFFLINE;
 
  // Don't try to send any more pings
  m_bCanPing = false;

  // Close the server socket
  INetSocket *s = gSocketMan.FetchSocket(m_nServerSocket);
  int nSD = m_nServerSocket;
  m_nServerSocket = -1;
  gSocketMan.DropSocket(s);
  gSocketMan.CloseSocket(nSD);

  
  // Close user sockets and update the daemon
  FOR_EACH_PROTO_USER_START(MSN_PPID, LOCK_W)
  {
    if (pUser->SocketDesc(ICQ_CHNxNONE) != -1)
    {
      gSocketMan.CloseSocket(pUser->SocketDesc(ICQ_CHNxNONE), false, true);
      pUser->ClearSocketDesc();
    }
    if (pUser->isOnline())
      gLicqDaemon->ChangeUserStatus(pUser, ICQ_STATUS_OFFLINE);
  }
  FOR_EACH_PROTO_USER_END
    
  ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_W);      
  gLicqDaemon->ChangeUserStatus(o, ICQ_STATUS_OFFLINE);
  gUserManager.DropOwner(o);
  //gLicqDaemon->pushPluginSignal(new LicqSignal(SIGNAL_LOGOFF, 0));
}

void CMSN::MSNAddUser(const UserId& userId)
{
  {
    LicqUserWriteGuard u(userId);
    if (u.isLocked())
    {
      u->SetEnableSave(false);
      u->SetUserEncoding("UTF-8");
      u->SetEnableSave(true);
      u->SaveLicqInfo();
    }
  }

  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pSend = new CPS_MSNAddUser(accountId.c_str(), CONTACT_LIST);
  SendPacket(pSend);
}

void CMSN::MSNRemoveUser(const UserId& userId)
{
  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pSend = new CPS_MSNRemoveUser(accountId.c_str(), CONTACT_LIST);
  SendPacket(pSend);
}

void CMSN::MSNRenameUser(const UserId& userId)
{
  string strNick;
  {
    LicqUserReadGuard u(userId);
    if (!u.isLocked())
      return;
    strNick = u->getAlias();
  }

  string strEncodedNick = Encode(strNick);
  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pSend = new CPS_MSNRenameUser(accountId.c_str(), strEncodedNick.c_str());
  SendPacket(pSend);
}

void CMSN::MSNGrantAuth(const UserId& userId)
{
  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pSend = new CPS_MSNAddUser(accountId.c_str(), ALLOW_LIST);
  SendPacket(pSend);
}

void CMSN::MSNUpdateUser(const string& alias)
{
  string strEncodedNick = Encode(alias);
  CMSNPacket *pSend = new CPS_MSNRenameUser(m_szUserName, strEncodedNick.c_str());
  SendPacket(pSend);
}

void CMSN::MSNBlockUser(const UserId& userId)
{
  {
    LicqUserWriteGuard u(userId);
    if (!u.isLocked())
      return;
    u->SetInvisibleList(true);
  }

  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pRem = new CPS_MSNRemoveUser(accountId.c_str(), ALLOW_LIST);
  gLog.Info("%sRemoving user %s from the allow list.\n", L_MSNxSTR, USERID_TOSTR(userId));
  SendPacket(pRem);
  CMSNPacket* pAdd = new CPS_MSNAddUser(accountId.c_str(), BLOCK_LIST);
  gLog.Info("%sAdding user %s to the block list.\n", L_MSNxSTR, USERID_TOSTR(userId));
  SendPacket(pAdd);
}

void CMSN::MSNUnblockUser(const UserId& userId)
{
  {
    LicqUserWriteGuard u(userId);
    if (!u.isLocked())
      return;
    u->SetInvisibleList(false);
  }

  string accountId = LicqUser::getUserAccountId(userId);
  CMSNPacket* pRem = new CPS_MSNRemoveUser(accountId.c_str(), BLOCK_LIST);
  gLog.Info("%sRemoving user %s from the block list\n", L_MSNxSTR, USERID_TOSTR(userId));
  SendPacket(pRem);
  CMSNPacket* pAdd = new CPS_MSNAddUser(accountId.c_str(), ALLOW_LIST);
  gLog.Info("%sAdding user %s to the allow list.\n", L_MSNxSTR, USERID_TOSTR(userId));
  SendPacket(pAdd);
}

void CMSN::MSNGetDisplayPicture(const string &strUser, const string &strMSNObject)
{
  // If we are invisible, this will result in an error, so don't allow it
  if (m_nStatus & ICQ_STATUS_FxPRIVATE)
    return;

  const char *szUser = const_cast<const char *>(strUser.c_str());
  CMSNPacket *pGetMSNDP = new CPS_MSNInvitation(szUser,
						m_szUserName,
						const_cast<char *>(strMSNObject.c_str()));
  CMSNP2PPacket *p = (CMSNP2PPacket *)(pGetMSNDP);
  CMSNDataEvent *pDataResponse = new CMSNDataEvent(MSN_DP_EVENT,
						   p->SessionId(), p->BaseId(),  strUser, 
                                                   m_szUserName, p->CallGUID(), this);
  WaitDataEvent(pDataResponse);
  gLog.Info("%sRequesting %s's display picture.\n", L_MSNxSTR, szUser);
  MSNSendInvitation(szUser, pGetMSNDP);
}


void CMSN::MSNPing()
{
  CMSNPacket *pSend = new CPS_MSNPing();
  SendPacket(pSend);
}

void *MSNPing_tep(void *p)
{
  CMSN *pMSN = (CMSN *)p;
  
  struct timeval tv;

  while (true)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    
    if (pMSN->WaitingPingReply())
    {
      pthread_mutex_lock(&(pMSN->mutex_ServerSocket));
      gLog.Info("%sPing timeout. Reconnecting...\n", L_MSNxSTR);
      pMSN->SetWaitingPingReply(false);
      pMSN->MSNLogoff();
      pMSN->MSNLogon(pMSN->serverAddress().c_str(), pMSN->serverPort());
      pthread_mutex_unlock(&(pMSN->mutex_ServerSocket));
    }
    else if (pMSN->CanSendPing())
    {
      pMSN->MSNPing();
      pMSN->SetWaitingPingReply(true);
    }

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    select(0, 0, 0, 0, &tv);

    pthread_testcancel();
  }  
  
  return 0;
}
