#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_KDE
#include <kfiledialog.h>
#include <qfiledialog.h>
#else
#include <qfiledialog.h>
#endif
#include <qtabbar.h>
#include <qwidgetstack.h>

#include "icqfunctions.h"
#include "chatacceptdlg.h"
#include "chatdlg.h"
#include "fileacceptdlg.h"
#include "filedlg.h"
#include "showawaymsgdlg.h"
#include "countrycodes.h"
#include "log.h"
#include "licq-locale.h"

#define MARGIN_LEFT 5
#define MARGIN_RIGHT 30

unsigned short ICQFunctions::s_nX = 100;
unsigned short ICQFunctions::s_nY = 100;

//-----ICQFunctions::constructor-------------------------------------------------------------------
ICQFunctions::ICQFunctions(CICQDaemon *s, CSignalManager *theSigMan,
                           unsigned long _nUin, bool _bIsOwner,
                           bool isAutoClose, QWidget *parent, const char *name)
   : QTabWidget(parent, name)
{
   server = s;
   sigman = theSigMan;
   icqEvent = NULL;
   m_nUin = _nUin;
   m_bIsOwner = _bIsOwner;

   strcpy(tabLabel[TAB_READ], _("View Event"));
   fcnTab[TAB_READ] = new QWidget(this, tabLabel[TAB_READ]);
   splRead = new QSplitter(QSplitter::Vertical, fcnTab[TAB_READ]);
   msgView = new MsgView(splRead);
   msgView->setFrameStyle( QFrame::WinPanel | QFrame::Sunken);
   mleRead = new MLEditWrap(true, splRead);
   mleRead->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);
   mleRead->setReadOnly(true);
   QValueList<int> v;
   v.append(60);
   v.append(180);
   splRead->setSizes(v);
   splRead->setResizeMode(msgView, QSplitter::KeepSize);
   splRead->setResizeMode(mleRead, QSplitter::Stretch);
   connect (msgView, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(printMessage(QListViewItem *)));

   strcpy(tabLabel[TAB_SEND], _("Send Event"));
   fcnTab[TAB_SEND] = new QWidget(this, tabLabel[TAB_SEND]);
   grpCmd = new QButtonGroup(_("Select Function"), fcnTab[TAB_SEND]);
   grpCmd->setExclusive(true);
   rdbMsg = new QRadioButton(_("Message"), grpCmd);
   rdbMsg->setChecked(true);
   rdbUrl = new QRadioButton(_("URL"), grpCmd);
   rdbUrl->setChecked(true);
   rdbAway = new QRadioButton(_("Check Auto Response"), grpCmd);
   rdbAway->setChecked(true);
   rdbChat = new QRadioButton(_("Chat Request"), grpCmd);
   rdbFile = new QRadioButton(_("File Transfer"), grpCmd);
   connect(grpCmd, SIGNAL(clicked(int)), this, SLOT(specialFcn(int)));
   lblDescription = new QLabel(fcnTab[TAB_SEND]);
   mleSend = new MLEditWrap(true, fcnTab[TAB_SEND]);
   mleSend->setFrameStyle( QFrame::WinPanel | QFrame::Sunken);
   lblItem = new QLabel(fcnTab[TAB_SEND]);
   edtItem = new QLineEdit(fcnTab[TAB_SEND]);
   chkSendServer = new QCheckBox(_("Send through server"), fcnTab[TAB_SEND]);
   chkUrgent = new QCheckBox(_("Urgent"), fcnTab[TAB_SEND]);
   chkSpoof = new QCheckBox(_("Spoof UIN:"), fcnTab[TAB_SEND]);
   edtSpoof = new QLineEdit(fcnTab[TAB_SEND]);
   edtSpoof->setEnabled(false);
   edtSpoof->setMaxLength(10);
   edtSpoof->setValidator(new QIntValidator(0, 2147483647, edtSpoof));
   connect(chkSpoof, SIGNAL(toggled(bool)), edtSpoof, SLOT(setEnabled(bool)));

   strcpy(tabLabel[TAB_BASICINFO], _("User Info"));
   fcnTab[TAB_BASICINFO] = new QWidget(this, tabLabel[TAB_BASICINFO]);
   nfoAlias = new CInfoField(5, 5, 45, 5, 110, _("Alias:"), false, fcnTab[TAB_BASICINFO]);
   nfoStatus = new CInfoField(180, 5, 35, 5, width() - MARGIN_RIGHT - 190,
                              _("Status:"), true, fcnTab[TAB_BASICINFO]);
   nfoUin = new CInfoField(5, 35, 45, 5, 110, _("UIN:"), true, fcnTab[TAB_BASICINFO]);
   nfoFirstName = new CInfoField(5, 65, 45, 5, 110, _("Name:"), false, fcnTab[TAB_BASICINFO]);
   nfoLastName = new CInfoField(170, 65, 0, 0, width() - MARGIN_RIGHT - 185,
                                NULL, false, fcnTab[TAB_BASICINFO]);
   nfoEMail = new CInfoField(5, 95, 45, 5, width() - MARGIN_RIGHT - 70,
                             _("EMail:"), false, fcnTab[TAB_BASICINFO]);
   nfoHistory = new CInfoField(5, 125, 45, 5, width() - MARGIN_RIGHT - 70,
                               _("History:"), false, fcnTab[TAB_BASICINFO]);
   nfoIp = new CInfoField(180, 35, 35, 5, width() - MARGIN_RIGHT - 230,
                          _("IP:"), true, fcnTab[TAB_BASICINFO]);
   chkAuthorization = new QCheckBox(_("Authorization Needed"), fcnTab[TAB_BASICINFO]);
   chkAuthorization->setEnabled(m_bIsOwner);

   strcpy(tabLabel[TAB_DETAILINFO], _("Details"));
   fcnTab[TAB_DETAILINFO] = new QWidget(this, tabLabel[TAB_DETAILINFO]);
   nfoAge = new CInfoField(5, 5, 45, 5, 100, _("Age:"), !m_bIsOwner, fcnTab[TAB_DETAILINFO]);
   nfoState = new CInfoField(5, 35, 45, 5, 100, _("State:"), !m_bIsOwner, fcnTab[TAB_DETAILINFO]);
   nfoCity = new CInfoField(5, 65, 45, 5, width() - MARGIN_RIGHT - 70, _("City:"), !m_bIsOwner, fcnTab[TAB_DETAILINFO]);
   lblCountry = new QLabel(_("Country:"), fcnTab[TAB_DETAILINFO]);
   cmbCountry = new CEComboBox(true, fcnTab[TAB_DETAILINFO]);
   cmbCountry->insertItem(_("Unspecified"));
   cmbCountry->insertItem(_("Unknown (unspecified)"));
   m_nUnknownCountryCode = 0xFFFF; // set the unknown value to default to unspecified
   cmbCountry->insertStrList(GetCountryList());
   cmbCountry->setEnabled(m_bIsOwner);
   nfoHomepage = new CInfoField(5, 125, 65, 5, width() - MARGIN_RIGHT - 90,
                                _("Homepage:"), !m_bIsOwner, fcnTab[TAB_DETAILINFO]);
   lblSex = new QLabel(_("Sex:"), fcnTab[TAB_DETAILINFO]);
   cmbSex = new CEComboBox(true, fcnTab[TAB_DETAILINFO]);
   cmbSex->insertItem(_("Unspecified"));
   cmbSex->insertItem(_("Female"));
   cmbSex->insertItem(_("Male"));
   cmbSex->setEnabled(m_bIsOwner);
   nfoPhone = new CInfoField(180, 35, 45, 5, width() - MARGIN_RIGHT - 245,
                             _("Phone:"), !m_bIsOwner, fcnTab[TAB_DETAILINFO]);
   boxAboutMsg = new QGroupBox (fcnTab[TAB_DETAILINFO]);
   boxAboutMsg->setFrameStyle(QFrame::Box | QFrame::Sunken);
   boxAboutMsg->setAlignment(AlignLeft);
   boxAboutMsg->setTitle(_("About"));
   mleAboutMsg = new MLEditWrap(true, boxAboutMsg);

   strcpy(tabLabel[TAB_HISTORY], _("History"));
   fcnTab[TAB_HISTORY] = new QWidget(this, tabLabel[TAB_HISTORY]);
   mleHistory = new MLEditWrap(true, fcnTab[TAB_HISTORY]);
   chkEditHistory = new QCheckBox(_("History read only"), fcnTab[TAB_HISTORY]);
   connect(chkEditHistory, SIGNAL(toggled(bool)), mleHistory, SLOT(setReadOnly(bool)));

   addTab(fcnTab[TAB_READ], tabLabel[TAB_READ]);
   addTab(fcnTab[TAB_SEND], tabLabel[TAB_SEND]);
   addTab(fcnTab[TAB_BASICINFO], tabLabel[TAB_BASICINFO]);
   addTab(fcnTab[TAB_DETAILINFO], tabLabel[TAB_DETAILINFO]);
   addTab(fcnTab[TAB_HISTORY], tabLabel[TAB_HISTORY]);

   chkAutoClose = new QCheckBox(_("Auto Close"), this);
   chkAutoClose->setChecked(isAutoClose);
   btnOk = new QPushButton("Ok", this);
   btnCancel = new QPushButton("Close", this);
   btnSave = new QPushButton(_("Save"), this);

   connect (mleSend, SIGNAL(signal_CtrlEnterPressed()), btnOk, SIGNAL(clicked()));
   connect (chkSpoof, SIGNAL(clicked()), this, SLOT(setSpoofed()));
   connect (this, SIGNAL(selected(const QString &)), this, SLOT(tabSelected(const QString &)));
   connect (sigman, SIGNAL(signal_doneUserFcn(ICQEvent *)), this, SLOT(doneFcn(ICQEvent *)));
   connect (sigman, SIGNAL(signal_updatedUser(unsigned long, unsigned short)), this, SLOT(slot_updatedUser(unsigned long, unsigned short)));
   connect (btnCancel, SIGNAL(clicked()), this, SLOT(close()));
   connect (btnOk, SIGNAL(clicked()), this, SLOT(callFcn()));
   connect (btnSave, SIGNAL(clicked()), this, SLOT(save()));

  setGeometry(s_nX, s_nY, 400, 360);
#ifdef TEST_POS
  printf("constructor: %d %d\n", x(), y());
#endif
  setMinimumSize(400, 360);
}

#define RM 6
#define LM 6
#define TM 6
#define BM 42

void ICQFunctions::resizeEvent(QResizeEvent *e)
{
  QTabWidget::resizeEvent(e);

  // Overload the stack and tab bar placement because Qt's sucks
  QWidgetStack *stack = Q_CHILD(this, QWidgetStack, "tab pages");
  QTabBar *tabs = Q_CHILD(this, QTabBar, "tab control");
  if (stack == NULL || tabs == NULL)
  {
    gLog.Error("%sICQFunctions::resizeEvent(): Unable to find widget stack or tab bar.", L_ERRORxSTR);
  }
  else
  {
    QSize t(tabs->sizeHint());
    int lw = stack->lineWidth();
	  tabs->setGeometry(QMAX(0, lw - 2) + LM, TM, t.width(), t.height());
	  stack->setGeometry(LM, t.height() - lw + TM, width() - (RM + LM), height() - t.height() + lw - (TM + BM));
  }

  splRead->setGeometry(MARGIN_LEFT, 5, width() - MARGIN_RIGHT, height() - 90);

  grpCmd->setGeometry(MARGIN_LEFT, 5, width() - MARGIN_RIGHT, 60);
  rdbMsg->setGeometry(10, 15, 90, 20);
  rdbUrl->setGeometry(115, 15, 90, 20);
  rdbAway->setGeometry(205, 15, 160, 20);
  rdbChat->setGeometry(10, 35, 100, 20);
  rdbFile->setGeometry(115, 35, 100, 20);
  lblDescription->setGeometry(5, 70, 120, 20);
  lblItem->setGeometry(MARGIN_LEFT, height() - 135, 80, 20);
  edtItem->setGeometry(70, height() - 135, width()-MARGIN_RIGHT-70+MARGIN_LEFT, 20);
  chkSendServer->setGeometry(5, height() - 110, 150, 20);
  chkUrgent->setGeometry(5, height() - 95, 80, 20);
  chkSpoof->setGeometry(170, height() - 100, 180, 20);
  edtSpoof->setGeometry(270, height() - 100, width() - MARGIN_RIGHT - 265, 20);
  mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - (lblItem->isVisible() ? 230 : 205));

  nfoAlias->setGeometry(5, 5, 45, 5, 110);
  nfoStatus->setGeometry(180, 5, 35, 5, width() - MARGIN_RIGHT - 215);
  nfoUin->setGeometry(5, 35, 45, 5, 110);
  nfoFirstName->setGeometry(5, 65, 45, 5, 110);
  nfoLastName->setGeometry(170, 65, 0, 0, width() - MARGIN_RIGHT - 165);
  nfoEMail->setGeometry(5, 95, 45, 5, width() - MARGIN_RIGHT - 50);
  nfoHistory->setGeometry(5, 125, 45, 5, width() - MARGIN_RIGHT - 50);
  nfoIp->setGeometry(180, 35, 35, 5, width() - MARGIN_RIGHT - 215);
  chkAuthorization->setGeometry(5, 150, 200, 20);

  nfoAge->setGeometry(5, 5, 45, 5, 100);
  nfoState->setGeometry(5, 35, 45, 5, 100);
  nfoCity->setGeometry(5, 65, 45, 5, width() - MARGIN_RIGHT - 50);
  lblCountry->setGeometry(5, 95, 48, 20);
  cmbCountry->setGeometry(55, 95, width() - MARGIN_RIGHT - 50, 20);
  nfoHomepage->setGeometry(5, 125, 65, 5, width() - MARGIN_RIGHT - 70);
  lblSex->setGeometry(180, 5, 45, 20);
  cmbSex->setGeometry(230, 5, width() - MARGIN_RIGHT - 225, 20);
  nfoPhone->setGeometry(180, 35, 45, 5, width() - MARGIN_RIGHT - 225);
  boxAboutMsg->setGeometry(5, 155, width() - MARGIN_RIGHT, height() - 235);
  mleAboutMsg->setGeometry(10, 15, boxAboutMsg->width() - 20, boxAboutMsg->height() - 25);

  mleHistory->setGeometry(MARGIN_LEFT, 5, width() - MARGIN_RIGHT, height() - 110);
  chkEditHistory->setGeometry(5, height() - 100, 150, 20);

  chkAutoClose->setGeometry(10, height() - 30, 180, 20);
  btnCancel->setGeometry(width() - 86, height() - 34, 80, 26);
  btnOk->setGeometry(btnCancel->x() - 86, btnCancel->y(), btnCancel->width(), btnCancel->height());
//  btnSave->setGeometry(btnOk->x() - 86, btnOk->y(), btnOk->width(), btnOk->height());
}


//-----ICQFunctions::keyPressEvent----------------------------------------------
void ICQFunctions::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Key_Enter || e->key() == Key_Return)
  {
    callFcn();
    return;
  }
  QTabWidget::keyPressEvent(e);
}

//-----ICQFunctions::setupTabs--------------------------------------------------
void ICQFunctions::setupTabs(int index)
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);

  // read tab
  for (short i = u->getNumMessages() - 1; i >= 0; i--)
     (void) new MsgViewItem(u->GetEvent(i), i, msgView);

  // print the first event if it's a message
  MsgViewItem *e = (MsgViewItem *)msgView->firstChild();
  if (e != NULL && (e->msg->SubCommand() == ICQ_CMDxSUB_MSG || e->msg->Command() == 0))
  {
     msgView->setSelected(e, true);
     gUserManager.DropUser(u);
     printMessage(e);
     u = gUserManager.FetchUser(m_nUin, LOCK_R);
  }

  // Send tab
  rdbAway->setEnabled(u->isAway());
  rdbFile->setEnabled(!u->getStatusOffline());
  rdbChat->setEnabled(!u->getStatusOffline());
  if (chkSendServer->isEnabled())
  {
    chkSendServer->setChecked(u->getSendServer() || (u->getStatusOffline() && u->SocketDesc() == -1));
  }
  if (u->Ip() == 0)
  {
    chkSendServer->setChecked(true);
    chkSendServer->setEnabled(false);
  }
  chkSpoof->setChecked(false);
  rdbMsg->setChecked(true);
  specialFcn(0);
  setTabEnabled(fcnTab[TAB_SEND], !m_bIsOwner);

  // Info tabs
  setBasicInfo(u);
  setExtInfo(u);

  // History tab
  mleHistory->setReadOnly(true);
  chkEditHistory->setChecked(true);

  bool bIsNew = u->getIsNew();
  gUserManager.DropUser(u);

  // mark the user as no longer new if they are new
  if (bIsNew)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_W);
    u->setIsNew(false);
    gUserManager.DropUser(u);
    emit signal_updatedUser();
  }
  show();
  // Post a resize event to force proper geometry setting, again because qt sucks
  resizeEvent(NULL);
#ifdef TEST_POS
  printf("just shown: %d %d\n", x(), y());
  move(100,100);
  printf("just shown 2: %d %d\n", x(), y());
#endif
  switch (index)
  {
  case 0: showPage(fcnTab[0]); break;
  case 1: showPage(fcnTab[1]); rdbMsg->setChecked(true); specialFcn(0); break;
  case 2: showPage(fcnTab[1]); rdbUrl->setChecked(true); specialFcn(1); break;
  case 3: showPage(fcnTab[1]); rdbChat->setChecked(true); specialFcn(3); break;
  case 4: showPage(fcnTab[1]); rdbFile->setChecked(true); specialFcn(4); break;
  case 6:
     showPage(fcnTab[1]);
     if (rdbAway->isEnabled())
     {
        rdbAway->setChecked(true);
        specialFcn(2);
        callFcn();
     }
     break;
  case 8: showPage(fcnTab[2]); break;
  case 9: showPage(fcnTab[3]); break;
  case 10: showPage(fcnTab[4]); break;
  }

}


//-----ICQFunctions::setBasicInfo-----------------------------------------------
void ICQFunctions::setBasicInfo(ICQUser *u)
{
  struct UserBasicInfo us;
  bool bDropUser = false;

  if (u == NULL)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_R);
    bDropUser = true;
  }
  chkAuthorization->setChecked(u->getAuthorization());
  u->getBasicInfo(us);
  if (bDropUser) gUserManager.DropUser(u);

  nfoAlias->setData(us.alias);
  nfoStatus->setData(us.status);
  nfoFirstName->setData(us.firstname);
  nfoLastName->setData(us.lastname);
  nfoEMail->setData(us.email);
  nfoHistory->setData(us.history);
  nfoUin->setData(us.uin);
  nfoIp->setData(us.ip_port);
  sprintf(m_sBaseTitle, "%s (%s)", us.alias, us.name);
  setCaption(m_sBaseTitle);
  setIconText(us.alias);
}


//-----ICQFunctions::setExtInfo-------------------------------------------------
void ICQFunctions::setExtInfo(ICQUser *u)
{
  struct UserExtInfo ud;
  bool bDropUser = false;

  if (u == NULL)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_R);
    bDropUser = true;
  }
  u->getExtInfo(ud);
  cmbSex->setCurrentItem(u->getSexNum());

  unsigned short i = GetIndexByCountryCode(u->getCountryCode());
  if (i == COUNTRY_UNSPECIFIED)
    cmbCountry->setCurrentItem(0);
  else if (i == COUNTRY_UNKNOWN)
  {
    char c[64];
    m_nUnknownCountryCode = u->getCountryCode();
    sprintf(c, _("Unknown (%d)"), m_nUnknownCountryCode);
    cmbCountry->changeItem(c, 1);
    cmbCountry->setCurrentItem(1);
  }
  else  // known
    cmbCountry->setCurrentItem(i + 2);

  if (bDropUser) gUserManager.DropUser(u);

  nfoCity->setData(ud.city);
  nfoState->setData(ud.state);
  nfoPhone->setData(ud.phone);
  nfoAge->setData(ud.age);
  nfoHomepage->setData(ud.homepage);
  mleAboutMsg->setText(ud.about);
}


//-----ICQFunctions::SendUrl---------------------------------------------------
void ICQFunctions::SendUrl(const char *url, const char *desc)
{
  showPage(fcnTab[1]);
  rdbUrl->setChecked(true);
  specialFcn(1);
  edtItem->setText(url);
  mleSend->setText(desc);
}

//-----ICQFunctions::SendFile--------------------------------------------------
void ICQFunctions::SendFile(const char *file, const char *desc)
{
  showPage(fcnTab[1]);
  rdbFile->setChecked(true);
  specialFcn(4);
  edtItem->setText(file);
  mleSend->setText(desc);
}

//-----ICQFunctions::tabSelected-------------------------------------------------------------------
void ICQFunctions::tabSelected(const QString &tab)
{
  if (tab == tabLabel[TAB_SEND])
  {
     mleSend->setFocus();
     btnOk->setText(_("Send"));
     btnSave->hide();
     currentTab = TAB_SEND;
  }
  else if (tab == tabLabel[TAB_BASICINFO])
  {
     btnOk->setText(_("Update"));
     btnSave->setText(_("Save"));
//     btnSave->show();
     currentTab = TAB_BASICINFO;
  }
  else if (tab == tabLabel[TAB_READ])
  {
     btnOk->setText(_("Ok"));
     btnSave->setText(_("Reply"));
//     m_bIsOwner ? btnSave->hide() : btnSave->show();
     currentTab = TAB_READ;
  }
  else if (tab == tabLabel[TAB_DETAILINFO])
  {
     btnOk->setText(_("Update"));
     btnSave->setText(_("Save"));
//     btnSave->show();
     currentTab = TAB_DETAILINFO;
  }
  else if (tab == tabLabel[TAB_HISTORY])
  {
     if (mleHistory->numLines() == 1) showHistory();  // if no history, then get it
     btnOk->setText(_("Ok"));
     btnSave->setText(_("Save"));
//     btnSave->show();
     currentTab = TAB_HISTORY;
  }
  else
  {
     btnOk->setText(_("Ok"));
  }
  btnSave->hide();
  
//  btnSave->setGeometry(width() - (btnOk->width() * 3 + 7 + 7 + 6), btnOk->y(), btnOk->width(), btnOk->height());
#if 1
  printf("just shown new page: %d %d\n", x(), y());
#endif
}


//-----slot_updatedUser---------------------------------------------------------
void ICQFunctions::slot_updatedUser(unsigned long _nUin, unsigned short _nUpdateType)
{
  if (m_nUin != _nUin) return;
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  switch (_nUpdateType)
  {
  case UPDATE_STATUS:
  {
    char szStatus[32];
    u->getStatusStr(szStatus);
    nfoStatus->setData(szStatus);
    if (u->Ip() == 0)
    {
      chkSendServer->setChecked(true);
      chkSendServer->setEnabled(false);
    }
    else
    {
      chkSendServer->setEnabled(true);
    }
    break;
  }
  case UPDATE_EVENTS:
  {
    MsgViewItem *e = (MsgViewItem *)msgView->firstChild();
    short index = -1;
    if (e != NULL)
    {
      do
      {
        if (e->index > index) index = e->index;
        e = (MsgViewItem *)e->nextSibling();
      } while (e != NULL);
    }
    index++;
    (void) new MsgViewItem(u->GetEvent(index), index, msgView);
    break;
  }
  case UPDATE_BASIC:
    setBasicInfo(u);
    break;
  case UPDATE_EXT:
    setExtInfo(u);
    break;
  }
  gUserManager.DropUser(u);
}


//-----ICQFunctions::printMessage-----------------------------------------------
void ICQFunctions::printMessage(QListViewItem *e)
{
  CUserEvent *m = ((MsgViewItem *)e)->msg;
  mleRead->clear();
  mleRead->append(m->Text());
  if (m->Command() == ICQ_CMDxTCP_START || m->Command() == ICQ_CMDxRCV_SYSxMSGxONLINE)
  {
    switch (m->SubCommand())
    {
    case ICQ_CMDxSUB_CHAT:  // accept or refuse a chat request
      (void) new CChatAcceptDlg(server, m_nUin, m->Sequence());
      break;
    case ICQ_CMDxSUB_FILE:  // accept or refuse a file transfer
      (void) new CFileAcceptDlg(server, m_nUin, (CEventFile *)m);
      break;
    case ICQ_CMDxSUB_URL:   // view a url
      if (server->getUrlViewer() != NULL && QueryUser(this, _("View URL?"), _("Yes"), _("No")) )
      {
        char szCmd[strlen(server->getUrlViewer()) + strlen(((CEventUrl *)m)->Url()) + 8];
        sprintf(szCmd, "%s %s &", server->getUrlViewer(), ((CEventUrl *)m)->Url());
        if (system(szCmd) != 0) gLog.Error("%sView URL failed.\n", L_ERRORxSTR);
      }
      break;
    case ICQ_CMDxSUB_REQxAUTH:
      if (QueryUser(this, _("Authorize?"), _("Yes"), _("No")))
        server->icqAuthorize( ((CEventAuth *)m)->Uin() );
      //...only ask if they aren't already there...
      if (QueryUser(this, _("Add?"), _("Yes"), _("No")))
        server->AddUserToList( ((CEventAuth *)m)->Uin());
      break;
    } // switch
  }  // if

  short index = ((MsgViewItem *)e)->index;
  if (index >= 0)   // the message had not been seen yet
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
    u->ClearEvent(index);
    gUserManager.DropUser(u);
    msgView->markRead(index);
    emit signal_updatedUser();
  }
}


//-----ICQFunctions::save-------------------------------------------------------
void ICQFunctions::save()
{
  switch (currentTab)
  {
  case TAB_READ:
    generateReply();
    break;
  case TAB_BASICINFO:
    saveBasicInfo();
    break;
  case TAB_DETAILINFO:
    saveExtInfo();
    break;
  case TAB_HISTORY:
    saveHistory();
    break;
  default:
    gLog.Warn("%sInternal error: ICQFunctions::save(): invalid tab - %s.\n",
              L_WARNxSTR, tabLabel[currentTab]);
    break;
  }
}


//-----ICQFunctions::saveBasicInfo----------------------------------------------
void ICQFunctions::saveBasicInfo()
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
  u->setEnableSave(false);
  u->setAlias(nfoAlias->text());
  u->setFirstName(nfoFirstName->text());
  u->setLastName(nfoLastName->text());
  u->setEmail(nfoEMail->text());
  u->setHistoryFile(nfoHistory->text());
  u->setAuthorization(chkAuthorization->isChecked());
  u->setEnableSave(true);
  u->saveBasicInfo();
  gUserManager.DropUser(u);
}


//-----ICQFunctions::saveExtInfo---------------------------------------------
void ICQFunctions::saveExtInfo()
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
  u->setEnableSave(false);
  u->setCity(nfoCity->text());
  u->setState(nfoState->text());
  unsigned short i = cmbCountry->currentItem();
  if (i == 0)
    u->setCountry(COUNTRY_UNSPECIFIED);
  else if (i == 1)
    u->setCountry(m_nUnknownCountryCode);
  else
    u->setCountry(GetCountryCodeByIndex(i - 2));
  u->setAge(atol(nfoAge->text()));
  u->setSex(cmbSex->currentItem());
  u->setPhoneNumber(nfoPhone->text());
  u->setHomepage(nfoHomepage->text());
  u->setAbout((const char *)mleAboutMsg->text());
  u->setEnableSave(true);
  u->saveExtInfo();
  gUserManager.DropUser(u);
}


//-----ICQFunctions::showHistory-------------------------------------------------------------------
void ICQFunctions::showHistory()
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  char *buf;
  u->GetHistory(buf);
  if (buf != NULL)
  {
    gUserManager.DropUser(u);
    mleHistory->setText(buf);
    mleHistory->goToEnd();
    delete []buf;
  }
}


//-----ICQFunctions::saveHistory-------------------------------------------------------------------
void ICQFunctions::saveHistory()
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  u->SaveHistory((const char *)mleHistory->text());
  gUserManager.DropUser(u);
}


//-----ICQFunctions::generateReply-----------------------------------------------------------------
void ICQFunctions::generateReply()
{
  char *q = quote();
	mleSend->clear();
	mleSend->append(q);
  delete q;
  showPage(fcnTab[1]);
}


//-----ICQFunctions::quote-------------------------------------------------------------------------
char *ICQFunctions::quote(void)
{
  char *q = new char[strlen((const char *)mleRead->text()) + (mleRead->numLines() * 2) + 4];
  char *s = q;
  for (int i = 0; i < mleRead->numLines(); i++)
  {
    *(s++) = '>';
    *(s++) = ' ';
	  strcpy(s, mleRead->textLine(i));
    s += strlen(mleRead->textLine(i));
	  *(s++) = '\n';
	}
	*s = '\0';
	return (q);
}


//-----ICQFunctions::setSpoofed--------------------------------------------------------------------
void ICQFunctions::setSpoofed()
{
  if (chkSpoof->isChecked())
  {
    if (!QueryUser(this, _("Spoofing messages is immoral and possibly illegal.\nIn clicking OK you absolve the author from any \nresponsibility for your actions.\nDo you want to continue?"),
                   _("Ok"), _("Cancel")))
    {
      chkSpoof->setChecked(false);
    }
  }
}


//-----ICQFunctions::specialFcn--------------------------------------------------------------------
void ICQFunctions::specialFcn(int theFcn)
{
   switch (theFcn)
   {
   case 0:
      lblItem->hide();
      edtItem->hide();
      lblDescription->setText(_("Message:"));
      mleSend->setEnabled(true);
      chkSendServer->setEnabled(true);
      mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - 205);
      break;
   case 1:
      lblItem->setText(_("URL:"));
      lblItem->show();
      edtItem->show();
      lblDescription->setText(_("Description:"));
      mleSend->setEnabled(true);
      chkSendServer->setEnabled(true);
      mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - 230);
      break;
   case 2:
      lblItem->hide();
      edtItem->hide();
      lblDescription->setText(_("Check Auto Response"));
      chkSendServer->setChecked(false);
      chkSendServer->setEnabled(false);
      mleSend->setEnabled(false);
      mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - 230);
      break;
   case 3:
      lblItem->hide();
      edtItem->hide();
      lblDescription->setText(_("Reason:"));
      mleSend->setEnabled(true);
      chkSendServer->setChecked(false);
      chkSendServer->setEnabled(false);
      mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - 205);
      break;
   case 4:
      lblItem->setText(_("Filename:"));
      lblItem->show();
      edtItem->show();
      lblDescription->setText(_("Description:"));
      chkSendServer->setChecked(false);
      chkSendServer->setEnabled(false);
      mleSend->setEnabled(true);
      mleSend->setGeometry(MARGIN_LEFT, 90, width() - MARGIN_RIGHT, height() - 230);
#ifdef USE_KDE
      QString f = QFileDialog::getOpenFileName(NULL, NULL, this);
#else
      QString f = QFileDialog::getOpenFileName(NULL, NULL, this);
#endif
      if (!f.isNull()) edtItem->setText(f);
      break;
   }
}


//-----ICQFunctions::callFcn-----------------------------------------------------------------------
void ICQFunctions::callFcn()
{
  // disable user input
  btnOk->setEnabled(false);
  btnCancel->setText(_("Cancel"));

  // do nothing if a command is already being processed
  if (icqEvent != NULL) return;

  switch (currentTab)
  {
  case TAB_READ: close(); return;
  case TAB_SEND:
  {
    unsigned short nMsgLen = strlen(mleSend->text());
    if (nMsgLen > MAX_MESSAGE_SIZE)
     {
        char theWarning[1024];
        sprintf (theWarning, _("Message is %d characters, over the ICQ98 limit of %d.  \nLicq, ICQ99, and other clones support longer messages \nhowever ICQ98 will not.  Continue?"), nMsgLen, MAX_MESSAGE_SIZE);
        if(!QueryUser(this, theWarning, _("Ok"), _("Cancel")))
        {
          btnOk->setEnabled(true);
          break;
        }
     }
     unsigned long uin = (chkSpoof->isChecked() ? atoi(edtSpoof->text()) : 0);

     if (rdbMsg->isChecked())  // send a message
     {
        ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
        u->setSendServer(chkSendServer->isChecked());
        gUserManager.DropUser(u);
        sprintf(m_sProgressMsg, _("Sending msg %s..."), (chkSendServer->isChecked() ? _("server") : _("direct")));
        icqEvent = server->icqSendMessage(m_nUin, mleSend->text(), (chkSendServer->isChecked() ? false : true),
                                          chkUrgent->isChecked() ? true : false, uin);
     }
     else if (rdbAway->isChecked()) // check away message
     {
        strcpy(m_sProgressMsg, _("Fetching..."));
        ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
        u->setShowAwayMsg(true);
        gUserManager.DropUser(u);
        icqEvent = server->icqFetchAutoResponse(m_nUin, uin);
     }
     else if (rdbUrl->isChecked()) // send URL
     {
        sprintf(m_sProgressMsg, _("Sending URL %s..."), (chkSendServer->isChecked() ? _("server") : _("direct")));
        icqEvent = server->icqSendUrl(m_nUin, edtItem->text(), mleSend->text(), (chkSendServer->isChecked() ? false : true),
                                      chkUrgent->isChecked() ? true : false, uin);
     }
     else if (rdbChat->isChecked())   // send chat request
     {
        sprintf(m_sProgressMsg, _("Sending chat request %s..."), (chkSendServer->isChecked() ? _("server") : _("direct")));
        icqEvent = server->icqChatRequest(m_nUin, mleSend->text(), (chkSendServer->isChecked() ? false : true),
                                          chkUrgent->isChecked() ? true : false, uin);
     }
     else if (rdbFile->isChecked())   // send file transfer
     {
        sprintf(m_sProgressMsg, _("Sending file transfer %s..."), (chkSendServer->isChecked() ? _("server") : _("direct")));
        icqEvent = server->icqFileTransfer(m_nUin, edtItem->text(), mleSend->text(), (chkSendServer->isChecked() ? false : true),
                                           chkUrgent->isChecked() ? true : false, uin);
     }

     if (icqEvent == NULL)
        doneFcn(NULL);

     break;
  }

  case TAB_BASICINFO:
     if ( m_bIsOwner && (!QueryUser(this, _("Update local or server information?"), _("Local"), _("Server"))) )
     {
        strcpy(m_sProgressMsg, _("Updating server..."));
        icqEvent = server->icqUpdateBasicInfo(nfoAlias->text(),
                                           nfoFirstName->text(),
                                           nfoLastName->text(),
                                           nfoEMail->text(),
                                           chkAuthorization->isChecked());
     }
     else
     {
        strcpy(m_sProgressMsg, _("Updating..."));
        icqEvent = server->icqUserBasicInfo(m_nUin);
     }
     break;
  case TAB_DETAILINFO:
     if ( m_bIsOwner && (!QueryUser(this, _("Update local or server information?"), _("Local"), _("Server"))) )
     {
        strcpy(m_sProgressMsg, _("Updating server..."));
        unsigned short i = cmbCountry->currentItem();
        unsigned short cc = ( i == 0 ? COUNTRY_UNSPECIFIED : (i == 1 ? m_nUnknownCountryCode : GetCountryCodeByIndex(i - 2)) );
        icqEvent = server->icqUpdateExtendedInfo(nfoCity->text(), cc,
                                         nfoState->text(), atol(nfoAge->text()),
                                         cmbSex->currentItem(), nfoPhone->text(),
                                         nfoHomepage->text(), mleAboutMsg->text());
     }
     else
     {
        strcpy(m_sProgressMsg, _("Updating..."));
        icqEvent = server->icqUserExtendedInfo(m_nUin);
     }
     break;
  case TAB_HISTORY: close(); return;
  }

  char title[256];
  sprintf(title, "%s  [%s]", m_sBaseTitle, m_sProgressMsg);
  setCaption(title);
}


//-----ICQFunctions::doneFcn----------------------------------------------------
void ICQFunctions::doneFcn(ICQEvent *e)
{
  if (e != icqEvent) return;
  bool isOk = (e->m_eResult == EVENT_ACKED || e->m_eResult == EVENT_SUCCESS);
  bool bForceOpen = false;

  char title[256];
  char result[12];
  switch (e->m_eResult)
  {
  case EVENT_ACKED:
  case EVENT_SUCCESS:
    strcpy(result, _("done"));
    break;
  case EVENT_FAILED:
    strcpy(result, _("failed"));
    break;
  case EVENT_TIMEDOUT:
    strcpy(result, _("timed out"));
    break;
  case EVENT_ERROR:
    strcpy(result, _("error"));
    break;
  default:
    break;
  }
  sprintf(title, "%s  [%s%s]", m_sBaseTitle, m_sProgressMsg, result);
  setCaption(title);
  btnOk->setEnabled(true);
  btnCancel->setCaption(_("Close"));
  icqEvent = NULL;

  if (isOk)
  {
    switch(e->m_nCommand)
    {
    case ICQ_CMDxTCP_START:
    {
      ICQUser *u = NULL;
      CUserEvent *ue = e->m_xUserEvent;
      if (e->m_nSubResult == ICQ_TCPxACK_RETURN)
      {
        char result[128], status[32];
        u = gUserManager.FetchUser(m_nUin, LOCK_R);
        u->getStatusStr(status);
        sprintf(result, "%s is in %s mode.\nSend \"urgent\" to ignore.", u->getAlias(), status);
        gUserManager.DropUser(u);
        QueryUser(this, result, "Ok", NULL);
        bForceOpen = true;
      }
      else if (e->m_nSubResult == ICQ_TCPxACK_REFUSE)
      {
        char result[128];
        u = gUserManager.FetchUser(m_nUin, LOCK_R);
        sprintf(result, "%s refused %s, send through server.", u->getAlias(), ue->Description());
        gUserManager.DropUser(u);
        QueryUser(this, result, "Ok", NULL);
        bForceOpen = true;
      }
      else if (e->m_nSubCommand == ICQ_CMDxSUB_CHAT || e->m_nSubCommand == ICQ_CMDxSUB_FILE)
      {
        struct SExtendedAck *ea = e->m_sExtendedAck;
        if (ea == NULL || ue == NULL)
        {
          gLog.Error("%sInternal error: ICQFunctions::doneFcn(): chat or file request acknowledgement without extended result.\n", L_ERRORxSTR);
          return;
        }
        if (!ea->bAccepted)
        {
           char result[128];
           u = gUserManager.FetchUser(m_nUin, LOCK_R);
           sprintf(result, _("%s%s with %s refused:\n%s%s"), L_TCPxSTR,
                   ue->Description(), u->getAlias(), L_BLANKxSTR,
                   ea->szResponse);
           gUserManager.DropUser(u);
           InformUser(this, result);
        }
        else
        {
          switch (e->m_nSubCommand)
          {
          case ICQ_CMDxSUB_CHAT:
          {
            ChatDlg *chatDlg = new ChatDlg(m_nUin, false, ea->nPort);
            chatDlg->show();
            break;
          }
          case ICQ_CMDxSUB_FILE:
          {
            CFileDlg *fileDlg = new CFileDlg(m_nUin,
                                             ((CEventFile *)ue)->Filename(),
                                             ((CEventFile *)ue)->FileSize(),
                                             false, ea->nPort);
            fileDlg->show();
            break;
          }
          default:
            break;
          } // case
        } // if accepted
      } // if file or chat
      else
      {
        u = gUserManager.FetchUser(m_nUin, LOCK_R);
        if (u->isAway() && u->ShowAwayMsg())
          (void) new ShowAwayMsgDlg(m_nUin);
        gUserManager.DropUser(u);
        mleSend->clear();
      }

      break;
    } // case
    case ICQ_CMDxSND_THRUxSERVER:
    {
      mleSend->clear();
      break;
    }

    case ICQ_CMDxSND_USERxGETINFO:
    case ICQ_CMDxSND_USERxGETDETAILS:
    case ICQ_CMDxSND_UPDATExBASIC:
    case ICQ_CMDxSND_UPDATExDETAIL:
    {
      bForceOpen = true;
      break;
    }

    default:
      break;

    }

    if (chkAutoClose->isChecked() && !bForceOpen) close();
  }

}

void ICQFunctions::closeEvent(QCloseEvent *e)
{
#ifdef TEST_POS
  printf("close event: %d %d\n", x(), y());
#endif
  if (icqEvent != NULL)
  {
    setCaption(m_sBaseTitle);
    server->CancelEvent(icqEvent);
    icqEvent = NULL;
    btnOk->setEnabled(true);
    btnCancel->setText(_("Close"));
  }
  else
  {
    s_nX = x();
    s_nY = y();
    e->accept();
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
    u->fcnDlg = NULL;
    gUserManager.DropUser(u);
    delete this;
  }
}


ICQFunctions::~ICQFunctions(void)
{
}

#include "moc/moc_icqfunctions.h"
