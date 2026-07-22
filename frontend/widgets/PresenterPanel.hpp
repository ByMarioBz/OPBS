/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#pragma once

#include <obs.hpp>

#include <QPointer>
#include <QWidget>

#include <memory>
#include <vector>

class QLabel;
class QGridLayout;
class QMenu;
class QPixmap;
class QTimer;
class QToolButton;
class OBSBasic;
class OBSProjector;
class OBSQTDisplay;
class ThumbnailView;

class PresenterPanel : public QWidget {
	Q_OBJECT

	struct MediaEntry {
		QString path;
		OBSSource source;
		QPointer<QToolButton> button;
		QPointer<ThumbnailView> thumbnailView;
		QPointer<QTimer> thumbnailPrepareTimer;
		QPointer<QTimer> thumbnailReleaseTimer;
		bool thumbnailShowing = false;
	};

	OBSBasic *main;
	OBSScene stageScene;
	obs_sceneitem_t *activeItem = nullptr;
	OBSSource activeSource;
	QPointer<OBSProjector> stageProjector;
	QPointer<OBSQTDisplay> preview;
	QPointer<QGridLayout> mediaGrid;
	QPointer<QLabel> emptyState;
	QPointer<QLabel> mediaCount;
	QPointer<QLabel> currentMedia;
	QPointer<QLabel> stageStatus;
	QPointer<QLabel> menuStageStatus;
	QPointer<QMenu> screensMenu;
	QPointer<QMenu> monitorMenu;
	QPointer<QWidget> originalCentralWidget;
	std::vector<std::unique_ptr<MediaEntry>> entries;
	int selectedMonitor = -1;
	bool initialized = false;
	bool stageShowing = false;

	static void RenderPreview(void *data, uint32_t cx, uint32_t cy);
	void BuildInterface();
	void BuildScreensMenu();
	void RefreshMonitorMenu();
	void AddMediaFile(const QString &path);
	void ActivateMedia(MediaEntry *entry);
	void UpdateStageStatus();
	QPixmap PlaceholderForSource(obs_source_t *source) const;
	void SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap);

private slots:
	void ImportMedia();
	void SelectMonitor();

public:
	explicit PresenterPanel(OBSBasic *main);
	~PresenterPanel() override;

	void Initialize();
	void Shutdown();
};
