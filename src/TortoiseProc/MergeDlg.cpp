// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

// MergeDlg.cpp : implementation file
//

#include "stdafx.h"

#include "Git.h"
#include "TortoiseProc.h"
#include "MergeDlg.h"
#include "AppUtils.h"
#include "HistoryDlg.h"

// CMergeDlg dialog

IMPLEMENT_DYNAMIC(CMergeDlg, CResizableStandAloneDialog)

CMergeDlg::CMergeDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CMergeDlg::IDD, pParent)
	, CChooseVersion(this)
	, m_pDefaultText(MAKEINTRESOURCE(IDS_PROC_AUTOGENERATEDBYGIT))
	, m_bSquash(BST_UNCHECKED)
	, m_bNoCommit(BST_UNCHECKED)
	, m_bLog(BST_UNCHECKED)
	, m_nPopupPasteLastMessage(0)
	, m_nPopupRecentMessage(0)
	, m_bNoFF(BST_UNCHECKED)
	, m_bFFonly(BST_UNCHECKED)
{
	CString mergeLog = g_Git.GetConfigValue(L"merge.log");
	int nLog = _wtoi(mergeLog);
	m_nLog = nLog > 0 ? nLog : 20;
	m_bSkipCurrentBranch = true;
}

CMergeDlg::~CMergeDlg()
{
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CHOOSE_VERSION_DDX;

	DDX_Check(pDX,IDC_CHECK_NOFF,this->m_bNoFF);
	DDX_Check(pDX,IDC_CHECK_SQUASH,this->m_bSquash);
	DDX_Check(pDX,IDC_CHECK_NOCOMMIT,this->m_bNoCommit);
	DDX_Check(pDX, IDC_CHECK_MERGE_LOG, m_bLog);
	DDX_Text(pDX, IDC_EDIT_MERGE_LOGNUM, m_nLog);
	DDX_Text(pDX, IDC_COMBO_MERGESTRATEGY, m_MergeStrategy);
	DDX_Text(pDX, IDC_COMBO_STRATEGYOPTION, m_StrategyOption);
	DDX_Text(pDX, IDC_EDIT_STRATEGYPARAM, m_StrategyParam);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_CHECK_FFONLY, m_bFFonly);
}


BEGIN_MESSAGE_MAP(CMergeDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CMergeDlg::OnBnClickedOk)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_SQUASH, &CMergeDlg::OnBnClickedCheckSquash)
	ON_BN_CLICKED(IDC_CHECK_MERGE_LOG, &CMergeDlg::OnBnClickedCheckMergeLog)
	ON_CBN_SELCHANGE(IDC_COMBO_MERGESTRATEGY, &CMergeDlg::OnCbnSelchangeComboMergestrategy)
	ON_CBN_SELCHANGE(IDC_COMBO_STRATEGYOPTION, &CMergeDlg::OnCbnSelchangeComboStrategyoption)
	ON_BN_CLICKED(IDC_CHECK_FFONLY, OnBnClickedCheckFFonlyOrNoFF)
	ON_BN_CLICKED(IDC_CHECK_NOFF, OnBnClickedCheckFFonlyOrNoFF)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

void CMergeDlg::ReloadHistoryEntries()
{
	m_History.Load(L"Software\\TortoiseGit\\History\\merge", L"logmsgs");
}

BOOL CMergeDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_MERGE_MESSAGE,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LOGMESSAGE,TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	AdjustControlSize(IDC_RADIO_BRANCH);
	AdjustControlSize(IDC_RADIO_TAGS);
	AdjustControlSize(IDC_RADIO_VERSION);
	AdjustControlSize(IDC_CHECK_SQUASH);
	AdjustControlSize(IDC_CHECK_NOFF);
	AdjustControlSize(IDC_CHECK_NOCOMMIT);
	AdjustControlSize(IDC_CHECK_MERGE_LOG);

	CheckRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION,IDC_RADIO_BRANCH);
	this->SetDefaultChoose(IDC_RADIO_BRANCH);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	m_ProjectProperties.ReadProps();

	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_cLogMessage.RegisterContextMenuHandler(this);

	m_cLogMessage.SetText(m_pDefaultText);
	m_cLogMessage.EnableWindow(!m_bSquash);
	if (m_bSquash)
		m_cLogMessage.SetAStyle(STYLE_DEFAULT, ::GetSysColor(COLOR_GRAYTEXT), ::GetSysColor(COLOR_BTNFACE));

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseGit\\MaxHistoryItems", 25));
	ReloadHistoryEntries();

	((CComboBox*)GetDlgItem(IDC_COMBO_MERGESTRATEGY))->AddString(L"resolve");
	((CComboBox*)GetDlgItem(IDC_COMBO_MERGESTRATEGY))->AddString(L"recursive");
	((CComboBox*)GetDlgItem(IDC_COMBO_MERGESTRATEGY))->AddString(L"ours");
	((CComboBox*)GetDlgItem(IDC_COMBO_MERGESTRATEGY))->AddString(L"subtree");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"ours");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"theirs");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"patience");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"ignore-space-change");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"ignore-all-space");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"ignore-space-at-eol");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"renormalize");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"no-renormalize");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"rename-threshold");
	((CComboBox*)GetDlgItem(IDC_COMBO_STRATEGYOPTION))->AddString(L"subtree");

	EnableSaveRestore(L"MergeDlg");
	GetDlgItem(IDOK)->SetFocus();

	if (m_initialRefName.IsEmpty())
	{
		CString currentBranch;
		if (g_Git.GetCurrentBranchFromFile(g_Git.m_CurrentDir, currentBranch))
			currentBranch.Empty();

		CString pr, pb;
		g_Git.GetRemotePushBranch(currentBranch, pr, pb);
		if (!pr.IsEmpty() && !pb.IsEmpty())
			m_initialRefName = L"remotes/" + pr + L'/' + pb;
	}

	InitChooseVersion(true);

	return FALSE;
}

// CMergeDlg message handlers


void CMergeDlg::OnBnClickedOk()
{
	this->UpdateData(TRUE);

	this->UpdateRevsionName();

	if (GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION) == IDC_RADIO_BRANCH && m_ChooseVersioinBranch.GetCurSel() == -1)
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_B_T_NOTEMPTY, IDS_APPNAME, MB_ICONEXCLAMATION);
		return;
	}

	this->m_strLogMesage = m_cLogMessage.GetText() ;
	if( m_strLogMesage == CString(this->m_pDefaultText) )
	{
		m_strLogMesage.Empty();
	}

	if (!m_strLogMesage.IsEmpty() && !m_bNoCommit)
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_strLogMesage);
		m_History.Save();
	}

	if (m_MergeStrategy != L"recursive")
		m_StrategyOption.Empty();
	if (m_StrategyOption != L"rename-threshold" && m_StrategyOption != L"subtree")
		m_StrategyParam.Empty();

	OnOK();
}

void CMergeDlg::OnCancel()
{
	UpdateData(TRUE);
	m_strLogMesage = m_cLogMessage.GetText();
	if (m_strLogMesage != CString(this->m_pDefaultText) && !m_strLogMesage.IsEmpty() && !m_bNoCommit)
	{
		ReloadHistoryEntries();
		m_History.AddEntry(m_strLogMesage);
		m_History.Save();
	}
	CResizableStandAloneDialog::OnCancel();
}

void CMergeDlg::OnDestroy()
{
	WaitForFinishLoading();
	__super::OnDestroy();
}

// CSciEditContextMenuInterface
void CMergeDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	ReloadHistoryEntries();
	//CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
	if (m_History.GetCount() > 0)
	{
		CString sMenuItemText;
		sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_PASTELASTMESSAGE);
		m_nPopupPasteLastMessage = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteLastMessage, sMenuItemText);

		sMenuItemText.LoadString(IDS_COMMITDLG_POPUP_LOGHISTORY);
		m_nPopupRecentMessage = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupRecentMessage, sMenuItemText);
	}
}

bool CMergeDlg::HandleMenuItemClick(int cmd, CSciEdit * pSciEdit)
{
	if (m_History.IsEmpty())
		return false;

	if (cmd == m_nPopupPasteLastMessage)
	{
		if (pSciEdit->GetText() == CString(m_pDefaultText))
			pSciEdit->SetText(L"");
		CString logmsg (m_History.GetEntry(0));
		pSciEdit->InsertText(logmsg);
		return true;
	}

	if (cmd == m_nPopupRecentMessage)
	{
		CHistoryDlg historyDlg;
		historyDlg.SetHistory(m_History);
		if (historyDlg.DoModal() != IDOK)
			return false;

		if (pSciEdit->GetText() == CString(m_pDefaultText))
			pSciEdit->SetText(L"");
		m_cLogMessage.InsertText(historyDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
		return true;
	}
	return false;
}

void CMergeDlg::OnBnClickedCheckSquash()
{
	UpdateData(TRUE);
	m_cLogMessage.EnableWindow(!m_bSquash);
	m_cLogMessage.SetAStyle(STYLE_DEFAULT, ::GetSysColor(m_bSquash ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT), ::GetSysColor(m_bSquash ? COLOR_BTNFACE : COLOR_WINDOW));
	GetDlgItem(IDC_CHECK_NOFF)->EnableWindow(!m_bSquash);
}

void CMergeDlg::OnBnClickedCheckMergeLog()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_EDIT_MERGE_LOGNUM)->EnableWindow(m_bLog);
}

void CMergeDlg::OnCbnSelchangeComboMergestrategy()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_COMBO_STRATEGYOPTION)->EnableWindow(m_MergeStrategy == L"recursive");
	GetDlgItem(IDC_EDIT_STRATEGYPARAM)->EnableWindow(m_MergeStrategy == L"recursive" ? m_StrategyOption == L"rename-threshold" || m_StrategyOption == L"subtree" : FALSE);
}

void CMergeDlg::OnCbnSelchangeComboStrategyoption()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_EDIT_STRATEGYPARAM)->EnableWindow(m_StrategyOption == L"rename-threshold" || m_StrategyOption == L"subtree");
}

void CMergeDlg::OnBnClickedCheckFFonlyOrNoFF()
{
	UpdateData();

	GetDlgItem(IDC_CHECK_FFONLY)->EnableWindow(!m_bNoFF);
	GetDlgItem(IDC_CHECK_NOFF)->EnableWindow(!m_bFFonly);
	GetDlgItem(IDC_CHECK_SQUASH)->EnableWindow(!m_bNoFF);
}

void CMergeDlg::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_cLogMessage.SetColors(true);
	m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
	m_cLogMessage.SetAStyle(STYLE_DEFAULT, ::GetSysColor(m_bSquash ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT), ::GetSysColor(m_bSquash ? COLOR_BTNFACE : COLOR_WINDOW));
}
