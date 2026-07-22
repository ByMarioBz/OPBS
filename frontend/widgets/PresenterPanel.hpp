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

class QAction;
class QCheckBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPixmap;
class QProgressBar;
class QSlider;
class QTimer;
class OBSBasic;
class OBSProjector;
class OBSQTDisplay;
class ThumbnailView;
typedef struct obs_volmeter obs_volmeter_t;

class PresenterPanel : public QWidget {
	Q_OBJECT

	struct MediaEntry {
		QString path;
		OBSSource source;
		QListWidgetItem *item = nullptr;
		QPointer<ThumbnailView> thumbnailView;
		QPointer<QTimer> thumbnailPrepareTimer;
		QPointer<QTimer> thumbnailReleaseTimer;
		bool thumbnailShowing = false;
	};

	OBSBasic *main;
	OBSScene stageScene;
	obs_sceneitem_t *activeItem = nullptr;
	OBSSource activeSource;
	OBSSource gainFilter;
	OBSSource compressorFilter;
	OBSSource limiterFilter;
	obs_volmeter_t *audioMeter = nullptr;
	QPointer<OBSProjector> stageProjector;
	QPointer<OBSQTDisplay> preview;
	QPointer<QListWidget> mediaList;
	QPointer<QLabel> emptyState;
	QPointer<QLabel> mediaCount;
	QPointer<QLabel> currentMedia;
	QPointer<QLabel> stageStatus;
	QPointer<QLabel> timeLabel;
	QPointer<QProgressBar> meterLeft;
	QPointer<QProgressBar> meterRight;
	QPointer<QSlider> mediaVolumeSlider;
	QPointer<QSlider> timelineSlider;
	QPointer<QCheckBox> stageToggle;
	QPointer<QAction> screensAction;
	QPointer<QAction> soundAction;
	QPointer<QWidget> originalCentralWidget;
	QPointer<QTimer> timelineTimer;
	std::vector<std::unique_ptr<MediaEntry>> entries;
	QString selectedMonitorName;
	QString audioDeviceName;
	QString audioDeviceId;
	QString selectedEffect;
	int selectedMonitor = -1;
	int mediaVolume = 100;
	int outputVolume = 100;
	int outputGain = 0;
	bool initialized = false;
	bool stageShowing = false;
	bool stageEnabled = false;
	bool timelineDragging = false;
	bool restoring = false;

	static void RenderPreview(void *data, uint32_t cx, uint32_t cy);
	static void AudioMeterUpdated(void *data, const float magnitude[], const float peak[], const float inputPeak[]);
	void BuildInterface();
	void BuildTopMenu();
	void AddMediaFile(const QString &path, bool save = true);
	void ActivateMedia(MediaEntry *entry);
	void UpdateStageStatus();
	void SetStageEnabled(bool enabled, bool save = true);
	void ResolveSelectedMonitor();
	void ShowScreensDialog();
	void ShowSoundDialog();
	void ApplyAudioSettings();
	void DetachAudioFilters();
	void RefreshTimeline();
	void ReorderEntriesFromList();
	void LoadSettings();
	void SaveSettings();
	QString SettingsPath() const;
	QPixmap PlaceholderForSource(obs_source_t *source) const;
	void SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap);
	void ImportPaths(const QStringList &paths);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;

private slots:
	void ImportMedia();

public:
	explicit PresenterPanel(OBSBasic *main);
	~PresenterPanel() override;

	void Initialize();
	void Shutdown();
};
