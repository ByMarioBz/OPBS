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
		QMainWindow, QDialog { background: #080A0E; color: #F4F7FA; }
		QWidget { color: #F4F7FA; font-family: "Segoe UI Variable"; font-size: 13px; }
		QMenuBar { background: #151820; color: #E9EDF2; border: 0; border-bottom: 1px solid #252B34;
			padding: 1px 4px; spacing: 2px; }
		QMenuBar::item { background: transparent; border-radius: 4px; padding: 5px 8px; }
		QMenuBar::item:selected { background: #262C36; color: white; }
		QMenuBar::item:pressed { background: #303845; }
		QMenu { background: #171B22; color: #F4F7FA; border: 1px solid #343C48; padding: 5px; }
		QMenu::item { border-radius: 4px; padding: 7px 28px 7px 10px; }
		QMenu::item:selected { background: #0A84FF; color: white; }
		QMenu::item:disabled { color: #687381; }
		QMenu::separator { height: 1px; background: #303743; margin: 5px 8px; }
		QLabel { color: #F4F7FA; }
		QGroupBox { background: #12161C; border: 1px solid #303743; border-radius: 8px;
			margin-top: 14px; padding: 14px 12px 10px 12px; font-weight: 600; }
		QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #F4F7FA; }
		QPushButton, QToolButton { min-height: 28px; color: #F4F7FA; background: #232A34;
			border: 1px solid #3A4451; border-radius: 6px; padding: 3px 11px; }
		QPushButton:hover, QToolButton:hover { background: #2D3541; border-color: #637080; }
		QPushButton:pressed, QToolButton:pressed { background: #1B2028; }
		QPushButton:focus, QToolButton:focus { border: 2px solid #0A84FF; padding: 2px 10px; }
		QPushButton:default { background: #0A84FF; border-color: #38A0FF; color: white; font-weight: 600; }
		QPushButton:default:hover { background: #2493FF; }
		QPushButton:disabled, QToolButton:disabled { color: #65707D; background: #171B21; border-color: #272E37; }
		QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QFontComboBox { min-height: 30px; background: #11151B;
			color: #F4F7FA; border: 1px solid #38414D; border-radius: 6px; padding: 2px 9px; selection-background-color: #0A84FF; }
		QLineEdit:hover, QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QFontComboBox:hover {
			border-color: #5B6878; }
		QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QFontComboBox:focus {
			border: 2px solid #0A84FF; padding: 1px 8px; }
		QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
			color: #65707D; background: #0D1015; border-color: #252B33; }
		QComboBox::drop-down { border: 0; width: 26px; }
		QComboBox QAbstractItemView { background: #171B22; color: #F4F7FA; border: 1px solid #3A4451;
			selection-background-color: #0A84FF; selection-color: white; outline: 0; }
		QCheckBox { spacing: 7px; }
		QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #586574;
			border-radius: 4px; background: #11151B; }
		QCheckBox::indicator:hover { border-color: #0A84FF; }
		QCheckBox::indicator:checked { background: #0A84FF; border-color: #38A0FF; }
		QSlider::groove:horizontal { height: 5px; background: #343D48; border-radius: 2px; }
		QSlider::sub-page:horizontal { background: #0A84FF; border-radius: 2px; }
		QSlider::handle:horizontal { background: #F8FAFC; border: 2px solid #0A84FF; width: 14px;
			margin: -6px 0; border-radius: 8px; }
		QSlider:focus { border: 1px solid #0A84FF; border-radius: 5px; }
		QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
		QScrollBar::handle:vertical { background: #45515F; min-height: 30px; border-radius: 4px; }
		QScrollBar::handle:vertical:hover { background: #657384; }
		QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
		QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: transparent; }
		QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
		QScrollBar::handle:horizontal { background: #45515F; min-width: 30px; border-radius: 4px; }
		QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
		QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: transparent; }
		QToolTip { background: #252C36; color: #F8FAFC; border: 1px solid #4A5664;
			border-radius: 4px; padding: 6px 8px; }
		QDialogButtonBox QPushButton { min-width: 84px; }
	)");
}

QString PresenterStyleSheet()
{
	return QStringLiteral(R"(
		#presenterPanel { background: #06080C; color: #F4F7FA; font-family: "Segoe UI Variable"; font-size: 13px; }
		#presenterHeader { background: #030405; border: 0; border-bottom: 1px solid #1D232B; }
		#presenterTitle { color: #F8FAFC; font-size: 19px; font-weight: 650; }
		#presenterSubtitle, #presenterMediaCount, #presenterTime { color: #98A3B1; }
		#presenterSectionLabel { color: #A9B3BF; font-size: 12px; font-weight: 600; padding: 3px 4px; }
		#presenterCountBadge { color: #B8C5D3; background: #171E27; border: 1px solid #2D3946;
			border-radius: 12px; padding: 5px 11px; font-size: 11px; }
		#presenterStatusChip { background: transparent; border: 0; }
		#presenterStatusDot { color: #58AFFF; font-size: 9px; }
		#presenterStatusText { color: #DCE4ED; font-size: 11px; font-weight: 600; }
		#presenterPreviewFrame, #presenterLibrary, #transmissionPreviewFrame {
			background: #0E1218; border: 0; }
		#presenterCurrent { color: #EEF2F6; font-weight: 600; }
		#presenterEmpty { color: #8D99A8; font-size: 14px; }
		#presenterHint { color: #8995A3; font-size: 11px; padding: 7px 3px; }
		#presenterDockWorkspace { background: #06080C; }
		QDockWidget { color: #DCE3EB; background: #0E1218; border: 1px solid #28323D;
			border-radius: 11px; font-weight: 600; font-size: 12px; }
		QDockWidget::title { background: transparent; border: 0; padding: 10px 14px 7px 14px;
			text-align: left; }
		QDockWidget::float-button { image: none; border: 0; width: 0; height: 0; }
		QMainWindow::separator { background: #06080C; width: 10px; height: 10px; }
		QListWidget#presenterMediaList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterMediaList::item { background: #151A21; color: #EEF2F6;
			border: 1px solid #2D3743; border-radius: 10px; padding: 7px; }
		QListWidget#presenterMediaList::item:hover { border-color: #526273; background: #1B222B; }
		QListWidget#presenterMediaList::item:selected { border: 2px solid #0A84FF; background: #10263C; }
		QListWidget#presenterMediaList:focus { border: 0; }
		#presenterFolderPanel { background: #0A0E13; border: 1px solid #252E39; border-radius: 9px; }
		QListWidget#presenterFolderList, QListWidget#presenterPresentationList {
			background: transparent; border: 0; outline: 0; }
		QListWidget#presenterFolderList::item, QListWidget#presenterPresentationList::item {
			padding: 10px 11px; margin: 2px; border: 1px solid transparent; border-radius: 8px; }
		QListWidget#presenterFolderList::item:hover, QListWidget#presenterPresentationList::item:hover {
			background: #1B2129; }
		QListWidget#presenterFolderList::item:selected, QListWidget#presenterPresentationList::item:selected {
			background: #15314B; border-color: #387DB5; color: white; }
		QListWidget#presenterFolderList:focus, QListWidget#presenterPresentationList:focus {
			border: 0; }
		QLineEdit#presenterSearch { min-height: 34px; background: #0A0E13; color: #EEF2F6;
			border: 1px solid #34404D; border-radius: 9px; padding: 2px 12px; }
		QLineEdit#presenterSearch:hover { border-color: #5E6B7A; }
		QLineEdit#presenterSearch:focus { border: 2px solid #0A84FF; padding: 1px 11px; }
		#presenterBibleControls { background: #0A0E13; border: 1px solid #252E39; border-radius: 9px; }
		QComboBox#presenterBibleSelector { min-height: 30px; background: #151A21; color: #EEF2F6;
			border: 1px solid #35404C; border-radius: 7px; padding: 2px 10px; min-width: 190px; }
		QComboBox#presenterBibleSelector::drop-down { border: 0; width: 28px; }
		QListWidget#presenterBibleList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterBibleList::item { background: #171C22; color: #EEF2F6; border: 1px solid #303945;
			border-radius: 9px; padding: 12px; }
		QListWidget#presenterBibleList::item:hover { border-color: #637182; background: #20262E; }
		QListWidget#presenterBibleList::item:selected { border: 2px solid #0A84FF; background: #132A41; }
		QProgressBar#presenterMeter { background: #252C35; border: 0; border-radius: 3px; max-height: 7px; }
		QProgressBar#presenterMeter::chunk { background: #32D583; border-radius: 3px; }
		QLabel#presenterScreenStatus { color: #F4F7FA; font-size: 11px; font-weight: 600; }
		QCheckBox#presenterStageToggle { spacing: 7px; color: white; background: #0A84FF;
			border: 1px solid #4AA9FF; border-radius: 12px; padding: 5px 10px; font-weight: 700; }
		QCheckBox#presenterStageToggle:hover { background: #2493FF; }
		QCheckBox#presenterStageToggle:focus { border: 2px solid white; padding: 4px 8px; }
		QCheckBox#presenterStageToggle:unchecked { background: #171D25; border-color: #3A4552; color: #C3CBD4; }
		QCheckBox#presenterStageToggle::indicator { width: 15px; height: 10px; border: 2px solid white;
			border-radius: 2px; background: transparent; }
		QCheckBox#presenterStageToggle::indicator:unchecked { border-color: #B8C1CB; }
		QToolButton#presenterTransport { background: #171D25; color: #F4F7FA; border: 1px solid #34404D;
			border-radius: 9px; min-width: 40px; min-height: 32px; font-size: 16px; font-weight: 700; }
		QToolButton#presenterTransport:hover { background: #23303D; border-color: #0A84FF; }
		QToolButton#presenterTransport:focus { border: 2px solid #0A84FF; }
		QToolButton#presenterTransport:checked { background: #0A84FF; border-color: #38A0FF; color: white; }
		QToolButton#presenterTransport:disabled { color: #65707D; background: #171B21; border-color: #272E37; }
		QToolButton#transmissionMode { min-height: 30px; background: #20262E; color: #F4F7FA;
			border: 1px solid #37424E; border-radius: 7px; padding: 3px 11px; font-weight: 600; }
		QToolButton#transmissionMode:hover { background: #2B333D; border-color: #0A84FF; }
		QToolButton#transmissionMode:focus { border: 2px solid #0A84FF; }
		QToolButton#transmissionMode:checked { background: #0A84FF; border-color: #38A0FF; }
		QToolButton#transmissionLive { min-height: 30px; background: #C93646; color: white; border: 1px solid #E35564;
			border-radius: 7px; padding: 3px 12px; font-weight: 700; }
		QToolButton#transmissionLive:hover { background: #D84252; }
		QToolButton#transmissionLive:focus { border: 2px solid white; }
		QToolButton#transmissionLive:checked { background: #E14959; }
		QToolButton#captureAdd { min-height: 30px; background: #0A84FF; color: white; border: 1px solid #38A0FF;
			border-radius: 7px; padding: 3px 13px; font-weight: 700; }
		QToolButton#captureAdd:hover { background: #2493FF; }
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
