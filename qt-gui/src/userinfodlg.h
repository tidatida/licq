// -*- c-basic-offset: 2 -*-
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

#ifndef USERINFODLG_H
#define USERINFODLG_H

#include <qwidget.h>

#include "licq_history.h"
#include "mainwin.h"

class QSpinBox;
class QLabel;
class QCheckBox;
class QPushButton;
class QTabWidget;
class QProgressBar;

class CInfoField;
class CICQDaemon;
class ICQEvent;
class CSignalManager;
class CMainWindow;
class CEInfoField;
class CEComboBox;
class ICQUser;
class CHistoryWidget;

class UserInfoDlg : public QWidget
{
  Q_OBJECT
public:
  enum Tab {
    GeneralInfo,
    MoreInfo,
    WorkInfo,
    AboutInfo,
    HistoryInfo,
    LastCountersInfo,
    InfoTabCount
  };

  UserInfoDlg(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
               unsigned long _nUin, QWidget* parent = 0);
#ifdef QT_PROTOCOL_PLUGIN
  UserInfoDlg(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
    const char *szId, unsigned long nPPID, QWidget *parent = 0_;
#endif
  virtual ~UserInfoDlg();

  unsigned long Uin() { return m_nUin; }
#ifdef QT_PROTOCOL_PLUGIN
  char *Id()  { return m_szId; }
  unsigned long PPID()  { return m_nPPID; }
#endif
  void showTab(int);
  bool isTabShown(int);

protected:
  // common stuff
  struct {
    QString label;
    QWidget *tab;
    bool loaded;
  } tabList[InfoTabCount];
  bool m_bOwner;
  int currentTab;
  unsigned long m_nUin;
#ifdef QT_PROTOCOL_PLUGIN
  char *m_szId;
  unsigned long m_nPPID;
#endif
  QString m_sProgressMsg;
  QString m_sBasic;
  CICQDaemon *server;
  CMainWindow *mainwin;
  CSignalManager *sigman;
  unsigned long icqEventTag;
  QTabWidget* tabs;
  QPushButton* btnMain1, *btnMain2, *btnMain3, *btnMain4;

  // User Info tab
  void CreateGeneralInfo();
  void InitGeneralInfo();
  CInfoField *nfoFirstName, *nfoLastName, *nfoEmailPrimary, *nfoEmailSecondary, *nfoEmailOld,
             *nfoAlias, *nfoIp, *nfoUin, *nfoCity, *nfoState,
             *nfoZipCode, *nfoAddress, *nfoCountry, *nfoFax, *nfoCellular,
             *nfoPhone, *nfoTime, *nfoStatus;
  CEComboBox *cmbCountry;
  QLabel *lblAuth;

  // More info
  void CreateMoreInfo();
  CInfoField *nfoAge, *nfoBirthday, *nfoLanguage[3], *nfoHomepage,
             *nfoGender;
  CEComboBox *cmbLanguage[3], *cmbGender;
  QSpinBox *spnBirthDay, *spnBirthMonth, *spnBirthYear;

  // Work info
  void CreateWorkInfo();
  CInfoField *nfoCompanyName, *nfoCompanyCity, *nfoCompanyState,
             *nfoCompanyAddress, *nfoCompanyZip, *nfoCompanyCountry, *nfoCompanyPhone, *nfoCompanyFax,
             *nfoCompanyHomepage, *nfoCompanyPosition, *nfoCompanyDepartment;
  CEComboBox *cmbCompanyCountry;

  // About
  void CreateAbout();
  QLabel *lblAbout;
  MLEditWrap *mleAbout;

  // Last Counters
  void CreateLastCountersInfo();
  CInfoField *nfoLastOnline, *nfoLastSent, *nfoLastRecv, *nfoLastCheckedAR, *nfoOnlineSince;

  // History tab
  void CreateHistory();
  void SetupHistory();
  void SaveHistory();
  CInfoField *nfoHistory;
  CHistoryWidget *mlvHistory;
  QLabel *lblHistory, *lblFilter;
  QLineEdit* lneFilter;
  QProgressBar* barFiltering;
  QCheckBox *chkHistoryReverse;
  HistoryList m_lHistoryList;
  HistoryListIter m_iHistorySIter;
  HistoryListIter m_iHistoryEIter;
  bool m_bHistoryReverse;
  unsigned short m_nHistoryIndex, m_nHistoryShowing;

  void SetGeneralInfo(ICQUser *);
  void SetMoreInfo(ICQUser *);
  void SetWorkInfo(ICQUser *);
  void SetAbout(ICQUser *);
  void SetLastCountersInfo(ICQUser *);
  void SaveGeneralInfo();
  void SaveMoreInfo();
  void SaveWorkInfo();
  void SaveAbout();

  virtual void keyPressEvent(QKeyEvent*);

protected slots:
  void ShowHistory();
  void ShowHistoryPrev();
  void ShowHistoryNext();
  void HistoryReverse(bool);
  void HistoryReload();
  void updateTab(const QString&);
  void updatedUser(CICQSignal*);
  void SaveSettings();
  void slotUpdate();
  void slotRetrieve();
  void doneFunction(ICQEvent*);
  void resetCaption();
#ifdef QT_PROTOCOL_PLUGIN
  void ShowUsermenu() { gMainWindow->SetUserMenuUser(m_szId, m_nPPID); }
#else
  void ShowUsermenu() { gMainWindow->SetUserMenuUin(m_nUin); }
#endif
  void slot_showHistoryTimer();

signals:
#ifdef QT_PROTOCOL_PLUGIN
  //void finished(const char *, unsigned long);
#endif
  void finished(unsigned long);
  void signal_updatedUser(CICQSignal *);

private:
  static bool chkContains(const char* text, const char* filter, int filterlen);
  QTimer *timer;
};

#endif
