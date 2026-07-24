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
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPixmap;
class QProgressBar;
class QSlider;
class QTimer;
class QToolButton;
class OBSBasic;
class OBSProjector;
class OBSQTDisplay;
class ThumbnailView;
typedef struct obs_volmeter obs_volmeter_t;

class PresenterPanel : public QWidget {
	Q_OBJECT

	struct MediaEntry {
		QString path;
		QString folderId;
		OBSSource source;
		QListWidgetItem *item = nullptr;
		QPointer<ThumbnailView> thumbnailView;
		bool isImage = false;
		bool thumbnailLoaded = false;
	};
	struct FolderEntry {
		QString id;
		QString name;
	};
	struct BibleVerse {
		QString text;
		QString reference;
		QString searchableText;
	};

	OBSBasic *main;
	OBSScene stageScene;
	obs_sceneitem_t *activeItem = nullptr;
	obs_sceneitem_t *bibleBackgroundItem = nullptr;
	obs_sceneitem_t *bibleVerseItem = nullptr;
	obs_sceneitem_t *bibleReferenceItem = nullptr;
	OBSSource activeSource;
	OBSSource bibleBackgroundSource;
	OBSSource bibleVerseSource;
	OBSSource bibleReferenceSource;
	MediaEntry *activeEntry = nullptr;
	OBSSource gainFilter;
	OBSSource compressorFilter;
	OBSSource limiterFilter;
	obs_volmeter_t *audioMeter = nullptr;
	QPointer<OBSProjector> stageProjector;
	QPointer<OBSQTDisplay> preview;
	QPointer<QListWidget> mediaList;
	QPointer<QListWidget> folderList;
	QPointer<QListWidget> presentationFolderList;
	QPointer<QListWidget> bibleResultsList;
	QPointer<QLineEdit> searchEdit;
	QPointer<QLineEdit> bibleSearchEdit;
	QPointer<QComboBox> bibleSelector;
	QPointer<QWidget> bibleControls;
	QPointer<QLabel> emptyState;
	QPointer<QLabel> mediaCount;
	QPointer<QLabel> currentMedia;
	QPointer<QLabel> stageStatus;
	QPointer<QLabel> timeLabel;
	QPointer<QProgressBar> meterLeft;
	QPointer<QProgressBar> meterRight;
	QPointer<QSlider> mediaVolumeSlider;
	QPointer<QSlider> timelineSlider;
	QPointer<QToolButton> playPauseButton;
	QPointer<QToolButton> loopButton;
	QPointer<QCheckBox> stageToggle;
	QPointer<QAction> editMenuAction;
	QPointer<QAction> fitToScreenAction;
	QPointer<QAction> screensAction;
	QPointer<QAction> soundAction;
	QPointer<QAction> bibleAction;
	QPointer<QWidget> originalCentralWidget;
	QPointer<QTimer> timelineTimer;
	QPointer<QTimer> seekTimer;
	std::vector<std::unique_ptr<MediaEntry>> entries;
	std::vector<FolderEntry> folders;
	std::vector<BibleVerse> bibleVerses;
	QString currentFolderId = "general";
	QString currentBiblePath;
	QString bibleFontFamily = "Arial";
	QString bibleTextAlignment = "center";
	QString bibleReferencePosition = "bottom-center";
	QString activeBibleText;
	QString activeBibleReference;
	QString selectedMonitorName;
	QString audioDeviceName;
	QString audioDeviceId;
	QString selectedEffect;
	int selectedMonitor = -1;
	int mediaVolume = 100;
	int outputVolume = 100;
	int outputGain = 0;
	int bibleFontSize = 96;
	bool initialized = false;
	bool stageShowing = false;
	bool stageActive = false;
	bool stageEnabled = false;
	bool loopCurrent = false;
	bool fitContentToScreen = false;
	bool timelineDragging = false;
	bool restoring = false;
	qint64 seekGuardUntil = 0;
	int64_t cachedDuration = 0;
	int pendingSeekValue = -1;

	static void RenderPreview(void *data, uint32_t cx, uint32_t cy);
	static void AudioMeterUpdated(void *data, const float magnitude[], const float peak[], const float inputPeak[]);
	static void MediaStarted(void *data, calldata_t *calldata);
	void BuildInterface();
	void BuildTopMenu();
	void AddMediaFile(const QString &path, const QString &folderId = QString(), bool save = true);
	bool EnsureSource(MediaEntry *entry);
	void ReleaseSource(MediaEntry *entry);
	void ActivateMedia(MediaEntry *entry);
	void ClearActiveMedia(MediaEntry *keepEntry = nullptr);
	void ClearBiblePresentation();
	void ProjectBibleVerse(const QString &text, const QString &reference);
	void RemoveMediaEntry(MediaEntry *entry);
	void UpdateStageStatus();
	void SetStageEnabled(bool enabled, bool save = true);
	void ResolveSelectedMonitor();
	void ShowScreensDialog();
	void ShowSoundDialog();
	void ShowBibleDialog();
	void ApplyAudioSettings();
	void ApplyLoopSetting();
	void ApplyActiveItemBounds();
	void DetachAudioFilters();
	void RefreshTimeline();
	void SeekToPendingPosition();
	void RebuildAudioMonitor(bool resetDevice = false);
	void TogglePlayPause();
	void PlayMedia();
	void PauseMedia();
	void StopMedia();
	void NextMedia();
	void PreviousMedia();
	void ReorderEntriesFromList();
	void ReorderFoldersFromList();
	void ApplyLibraryFilter();
	void ApplyBibleFilter();
	void LoadBibleCatalog(const QString &preferredPath = QString());
	void LoadBibleTranslation(int index);
	bool ParseBibleFile(const QString &path, std::vector<BibleVerse> &verses, QString *error = nullptr) const;
	bool ImportBibleFile(const QString &path);
	QString BibleDirectoryPath() const;
	void CreateFolder();
	void RenameFolder();
	void MoveMediaToFolder(const QStringList &paths, const QString &folderId);
	QString SelectedFolderId() const;
	void LoadSettings();
	void SaveSettings();
	QString SettingsPath() const;
	QPixmap PlaceholderForType(const char *sourceType) const;
	void SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap);
	void LoadThumbnail(MediaEntry *entry);
	void LoadVisibleThumbnails();
	void ImportPaths(const QStringList &paths, const QString &folderId = QString());

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
