/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#include "OpbsDesignSystem.hpp"

#include <QMainWindow>
#include <QMenuBar>

namespace OpbsDesignSystem {

QString ApplicationStyleSheet()
{
	return QStringLiteral(R"(
		QMainWindow, QDialog { background: #0B0B0E; color: #F5F5F7; }
		QDialog#opbsSettingsDialog { background: #101014; }
		QFileDialog { background: #101014; }
		QFileDialog QTreeView, QFileDialog QListView { background: #141418; color: #F5F5F7;
			border: 1px solid #303038; border-radius: 8px; alternate-background-color: #19191E;
			selection-background-color: #0969BD; selection-color: white; outline: 0; }
		QFileDialog QHeaderView::section { background: #1D1D22; color: #DEDEE3; border: 0;
			border-bottom: 1px solid #303038; padding: 6px; }
		QLabel#opbsDialogTitle { color: #F5F5F7; font-size: 22px; font-weight: 650; }
		QLabel#opbsDialogSubtitle { color: #A6A6B0; font-size: 13px; }
		QWidget { color: #F5F5F7; font-family: "Segoe UI Variable"; font-size: 13px; }
		QMenuBar { min-height: 30px; background: #17171B; color: #E8E8ED; border: 0;
			border-bottom: 1px solid #29292F; padding: 2px 8px; spacing: 3px; }
		QMenuBar::item { background: transparent; border-radius: 7px; padding: 6px 10px; }
		QMenuBar::item:selected { background: #24242A; color: white; }
		QMenuBar::item:pressed { background: #303038; }
		QMenu { background: #1D1D22; color: #F5F5F7; border: 1px solid #3A3A43;
			border-radius: 10px; padding: 6px; }
		QMenu::item { border-radius: 7px; padding: 8px 30px 8px 11px; }
		QMenu::item:selected { background: #0969BD; color: white; }
		QMenu::item:disabled { color: #6F6F78; }
		QMenu::separator { height: 1px; background: #303038; margin: 5px 8px; }
		QMenu#opbsContextMenu { background: #202024; color: #F5F5F7; border: 1px solid #3A3A43;
			border-radius: 12px; padding: 8px; }
		QMenu#opbsContextMenu::item { min-height: 30px; border: 0; border-radius: 8px;
			padding: 6px 32px 6px 34px; margin: 1px 0; }
		QMenu#opbsContextMenu::item:selected { background: #34343B; color: white; }
		QMenu#opbsContextMenu::item:disabled { color: #777780; }
		QMenu#opbsContextMenu::icon { left: 9px; }
		QMenu#opbsContextMenu::separator { height: 1px; background: #3A3A42; margin: 7px 9px; }
		QMenu#opbsContextMenu::right-arrow { width: 7px; height: 7px; margin-right: 9px; }
		QLabel { color: #F5F5F7; }
		QGroupBox { background: #18181D; border: 1px solid #303038; border-radius: 13px;
			margin-top: 18px; padding: 17px 14px 12px 14px; font-weight: 600; }
		QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #F5F5F7; }
		QPushButton, QToolButton { min-height: 30px; color: #F5F5F7; background: #24242A;
			border: 1px solid #3A3A43; border-radius: 10px; padding: 3px 12px; }
		QPushButton:hover, QToolButton:hover { background: #2D2D34; border-color: #666672; }
		QPushButton:pressed, QToolButton:pressed { background: #1B1B20; }
		QPushButton:focus, QToolButton:focus { border: 2px solid #5AAEFF; padding: 2px 10px; }
		QPushButton:default { background: #0969BD; border-color: #5AAEFF; color: white; font-weight: 600; }
		QPushButton:default:hover { background: #0B76D7; }
		QPushButton:disabled, QToolButton:disabled { color: #6F6F78; background: #19191E; border-color: #29292F; }
		QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QFontComboBox { min-height: 32px; background: #1C1C21;
			color: #F5F5F7; border: 1px solid #383841; border-radius: 10px; padding: 2px 10px;
			selection-background-color: #0969BD; }
		QLineEdit:hover, QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QFontComboBox:hover {
			border-color: #62626D; }
		QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QFontComboBox:focus {
			border: 2px solid #5AAEFF; padding: 1px 8px; }
		QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
			color: #6F6F78; background: #0D0D10; border-color: #29292F; }
		QComboBox::drop-down { border: 0; width: 26px; }
		QComboBox QAbstractItemView { background: #19191E; color: #F5F5F7; border: 1px solid #3A3A43;
			selection-background-color: #0969BD; selection-color: white; outline: 0; }
		QListWidget#opbsSettingsSidebar { background: #18181D; border: 1px solid #303038;
			border-radius: 13px; padding: 7px; outline: 0; }
		QListWidget#opbsSettingsSidebar::item { min-height: 32px; border: 0; border-left: 3px solid transparent;
			border-radius: 9px; padding: 5px 9px; margin: 2px; }
		QListWidget#opbsSettingsSidebar::item:hover { background: #24242A; }
		QListWidget#opbsSettingsSidebar::item:selected { background: #2A2A30; border-left: 3px solid #5AAEFF;
			color: #F5F5F7; }
		QStackedWidget#opbsSettingsPages { background: #141418; border: 1px solid #2C2C33;
			border-radius: 14px; }
		QScrollArea { background: transparent; border: 0; }
		QScrollArea > QWidget > QWidget { background: transparent; }
		QCheckBox { spacing: 7px; }
		QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #5B5B66;
			border-radius: 4px; background: #121216; }
		QCheckBox::indicator:hover { border-color: #5AAEFF; }
		QCheckBox::indicator:checked { background: #0969BD; border-color: #5AAEFF; }
		QSlider::groove:horizontal { height: 5px; background: #34343C; border-radius: 2px; }
		QSlider::sub-page:horizontal { background: #2997FF; border-radius: 2px; }
		QSlider::handle:horizontal { background: #F5F5F7; border: 2px solid #2997FF; width: 14px;
			margin: -6px 0; border-radius: 8px; }
		QSlider:focus { border: 1px solid #5AAEFF; border-radius: 5px; }
		QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
		QScrollBar::handle:vertical { background: #484851; min-height: 30px; border-radius: 4px; }
		QScrollBar::handle:vertical:hover { background: #676772; }
		QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
		QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: transparent; }
		QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
		QScrollBar::handle:horizontal { background: #484851; min-width: 30px; border-radius: 4px; }
		QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
		QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: transparent; }
		QToolTip { background: #24242A; color: #F5F5F7; border: 1px solid #4A4A54;
			border-radius: 8px; padding: 7px 9px; }
		QDialogButtonBox { border-top: 1px solid #29292F; padding-top: 14px; }
		QDialogButtonBox QPushButton { min-width: 92px; min-height: 32px; }
	)");
}

QString PresenterStyleSheet()
{
	return QStringLiteral(R"(
		#presenterPanel { background: #070709; color: #F5F5F7; font-family: "Segoe UI Variable"; font-size: 13px; }
		#presenterHeader { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
			stop:0 #151519, stop:1 #101013); border: 0; border-bottom: 1px solid #29292F; }
		#presenterTitle { color: #F5F5F7; font-size: 19px; font-weight: 650; }
		#presenterSubtitle, #presenterMediaCount, #presenterTime { color: #A6A6B0; }
		#presenterSectionLabel { color: #B1B1BA; font-size: 12px; font-weight: 600; padding: 3px 4px; }
		#presenterCountBadge { color: #C6C6CE; background: #222228; border: 0;
			border-radius: 13px; padding: 6px 12px; font-size: 11px; }
		#presenterStatusChip { background: #1C1C21; border: 1px solid #2D2D34; border-radius: 10px; }
		#presenterStatusDot { color: #5AAEFF; font-size: 9px; }
		#presenterStatusText { color: #DEDEE3; font-size: 11px; font-weight: 600; }
		#presenterPreviewFrame, #presenterLibrary, #transmissionPreviewFrame {
			background: #121216; border: 0; }
		#presenterCurrent { color: #EEEEF2; font-weight: 600; }
		#presenterEmpty { color: #9696A1; font-size: 14px; }
		#presenterHint { color: #92929D; font-size: 11px; padding: 7px 3px; }
		#presenterDockWorkspace { background: #070709; }
		QDockWidget { color: #DEDEE3; background: #121216; border: 1px solid #2A2A31;
			border-radius: 11px; font-weight: 600; font-size: 12px; }
		#opbsDockTitleBar { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
			stop:0 #1B1B20, stop:1 #16161A); border: 0; border-bottom: 1px solid #24242A;
			border-top-left-radius: 10px; border-top-right-radius: 10px; }
		#opbsDockTitleText { color: #EBEBEF; font-size: 13px; font-weight: 650; }
		#opbsDockTitleIcon { background: transparent; border: 0; }
		QMainWindow::separator { background: #070709; width: 10px; height: 10px; }
		QListWidget#presenterMediaList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterMediaList::item { background: #1A1A1F; color: #EEEEF2;
			border: 1px solid #2D2D34; border-radius: 12px; padding: 8px; }
		QListWidget#presenterMediaList::item:hover { border-color: #51515B; background: #24242A; }
		QListWidget#presenterMediaList::item:selected { border: 2px solid #5AAEFF; background: #202E3C; }
		QListWidget#presenterMediaList:focus { border: 0; }
		#presenterFolderPanel { background: #18181D; border: 1px solid #2C2C33; border-radius: 13px; }
		QListWidget#presenterFolderList, QListWidget#presenterPresentationList {
			background: transparent; border: 0; outline: 0; }
		QListWidget#presenterFolderList::item, QListWidget#presenterPresentationList::item {
			padding: 11px 12px; margin: 2px; border: 0; border-left: 3px solid transparent;
			border-radius: 9px; }
		QListWidget#presenterFolderList::item:hover, QListWidget#presenterPresentationList::item:hover {
			background: #222228; }
		QListWidget#presenterFolderList::item:selected, QListWidget#presenterPresentationList::item:selected {
			background: #2A2A30; border-left: 3px solid #5AAEFF; color: white; }
		QListWidget#presenterFolderList:focus, QListWidget#presenterPresentationList:focus {
			border: 0; }
		QLineEdit#presenterSearch { min-height: 38px; max-height: 38px; background: #1C1C21; color: #EEEEF2;
			border: 1px solid #34343C; border-radius: 12px; padding: 0 11px; }
		QLineEdit#presenterSearch:hover { border-color: #62626D; }
		QLineEdit#presenterSearch:focus { border: 2px solid #5AAEFF; padding: 0 9px; }
		#presenterBibleControls { background: #18181D; border: 1px solid #2C2C33; border-radius: 12px; }
		QComboBox#presenterBibleSelector { min-height: 32px; background: #24242A; color: #EEEEF2;
			border: 1px solid #3A3A43; border-radius: 10px; padding: 2px 11px; min-width: 190px; }
		QComboBox#presenterBibleSelector::drop-down { border: 0; width: 28px; }
		QListWidget#presenterBibleList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterBibleList::item { background: #19191E; color: #EEEEF2; border: 1px solid #303038;
			border-radius: 10px; padding: 12px; }
		QListWidget#presenterBibleList::item:hover { border-color: #62626D; background: #232329; }
		QListWidget#presenterBibleList::item:selected { border: 2px solid #5AAEFF; background: #173653; }
		QProgressBar#presenterMeter { background: #2B2B32; border: 0; border-radius: 3px; max-height: 7px; }
		QProgressBar#presenterMeter::chunk { background: #35C98A; border-radius: 3px; }
		QLabel#presenterScreenStatus { color: #F5F5F7; font-size: 11px; font-weight: 600; }
		QCheckBox#presenterStageToggle { spacing: 7px; color: white; background: #0969BD;
			border: 1px solid #5AAEFF; border-radius: 13px; padding: 5px 10px; font-weight: 700; }
		QCheckBox#presenterStageToggle:hover { background: #0B76D7; }
		QCheckBox#presenterStageToggle:focus { border: 2px solid white; padding: 4px 8px; }
		QCheckBox#presenterStageToggle:unchecked { background: #1B1B20; border-color: #3A3A43; color: #C6C6CE; }
		QCheckBox#presenterStageToggle::indicator { width: 15px; height: 10px; border: 2px solid white;
			border-radius: 2px; background: transparent; }
		QCheckBox#presenterStageToggle::indicator:unchecked { border-color: #B8B8C0; }
		#presenterTransportGroup, #transmissionModeGroup { background: #1C1C21; border: 1px solid #34343C;
			border-radius: 13px; }
		QToolButton#opbsIconButton { min-width: 36px; min-height: 34px; background: #24242A;
			border: 1px solid #3A3A43; border-radius: 10px; padding: 2px; font-weight: 700; }
		QToolButton#opbsIconButton:hover { background: #303038; border-color: #62626D; }
		QToolButton#opbsIconButton:focus { border: 2px solid #5AAEFF; }
		QToolButton#presenterTransport { background: transparent; color: #F5F5F7; border: 0;
			border-radius: 9px; min-width: 40px; min-height: 32px; font-size: 16px; font-weight: 700; }
		QToolButton#presenterTransport:hover { background: #303038; border: 0; }
		QToolButton#presenterTransport:focus { border: 2px solid #5AAEFF; }
		QToolButton#presenterTransport:checked { background: #0969BD; border: 0; color: white; }
		QToolButton#presenterTransport:disabled { color: #6F6F78; background: transparent; border: 0; }
		QToolButton#transmissionMode { min-height: 34px; background: transparent; color: #F5F5F7;
			border: 0; border-radius: 9px; padding: 3px 13px; font-weight: 600; }
		QToolButton#transmissionMode:hover { background: #303038; border: 0; }
		QToolButton#transmissionMode:focus { border: 2px solid #5AAEFF; }
		QToolButton#transmissionMode:checked { background: #0969BD; border: 0; }
		#transmissionStatusStrip { background: #18181D; border: 1px solid #2C2C33; border-radius: 11px; }
		QLabel#transmissionStatusChip { color: #A6A6B0; background: #222228; border: 1px solid #303038;
			border-radius: 9px; padding: 5px 9px; font-size: 11px; font-weight: 600; }
		QLabel#transmissionStatusChip[state="live"] { color: #FF7A86; background: #32191F; border-color: #71303A; }
		QLabel#transmissionStatusChip[state="recording"] { color: #FF9BA4; background: #32191F; border-color: #71303A; }
		QLabel#transmissionStatusChip[state="good"] { color: #73D9A9; background: #142A22; border-color: #285A45; }
		QLabel#transmissionStatusChip[state="warning"] { color: #F3C969; background: #302714; border-color: #655426; }
		QLabel#transmissionStatusChip[state="bad"] { color: #FF7A86; background: #32191F; border-color: #71303A; }
		QToolButton#transmissionLive { min-height: 40px; color: white; border-radius: 11px;
			padding: 3px 17px; font-weight: 700; }
		QToolButton#transmissionLive[action="stream"] { background: #B93646; border: 1px solid #D95B68; }
		QToolButton#transmissionLive[action="record"] { background: #29292F; border: 1px solid #555560; }
		QToolButton#transmissionLive[action="stream"]:hover { background: #CC4252; }
		QToolButton#transmissionLive[action="record"]:hover { background: #36363E; border-color: #777783; }
		QToolButton#transmissionLive:focus { border: 2px solid white; }
		QToolButton#transmissionLive:checked { background: #D94B5B; border-color: #FF929D; }
		QToolButton#captureAdd { min-height: 30px; background: #0969BD; color: white; border: 1px solid #5AAEFF;
			border-radius: 8px; padding: 3px 13px; font-weight: 700; }
		QToolButton#captureAdd:hover { background: #0B76D7; }
		QToolButton#captureAdd:focus { border: 2px solid white; }
	)");
}

void ApplyToMainWindow(QMainWindow *window)
{
	if (!window)
		return;
	window->setStyleSheet(ApplicationStyleSheet());
	if (window->menuBar()) {
		window->menuBar()->setNativeMenuBar(false);
		window->menuBar()->setAccessibleName(QObject::tr("Menú principal de OPBS"));
	}
}

} // namespace OpbsDesignSystem
