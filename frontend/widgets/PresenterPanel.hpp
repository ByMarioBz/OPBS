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
class QSplitter;
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
	struct StreamDestination {
		QString service = "YouTube";
		QString server;
		QString key;
		bool enabled = true;
	};
	enum class TransmissionView { Cameras, Presenter, CamerasAndPresenter };

	OBSBasic *main;
	OBSScene stageScene;
	OBSScene transmissionScene;
	OBSScene transmissionCameraScene;
	OBSScene transmissionPresenterScene;
	obs_sceneitem_t *activeItem = nullptr;
	obs_sceneitem_t *transmissionPresenterItem = nullptr;
	obs_sceneitem_t *transmissionCameraItem = nullptr;
	obs_sceneitem_t *transmissionPresenterOnlyItem = nullptr;
	obs_sceneitem_t *transmissionCameraOnlyItem = nullptr;
	obs_sceneitem_t *transmissionBackgroundItem = nullptr;
	obs_sceneitem_t *bibleBackgroundItem = nullptr;
	obs_sceneitem_t *bibleVerseItem = nullptr;
	obs_sceneitem_t *bibleReferenceItem = nullptr;
	OBSSource activeSource;
	OBSSource cameraSource;
	OBSSource transmissionTransition;
	OBSSource transmissionBackgroundSource;
	OBSSource transmissionPresenterAudioSource;
	OBSSource transmissionInputSource;
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
	QPointer<OBSQTDisplay> transmissionPreview;
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
	QPointer<QSplitter> mainSplitter;
	QPointer<QToolButton> playPauseButton;
	QPointer<QToolButton> loopButton;
	QPointer<QToolButton> streamButton;
	QPointer<QToolButton> recordButton;
	QPointer<QToolButton> camerasViewButton;
	QPointer<QToolButton> presenterViewButton;
	QPointer<QToolButton> combinedViewButton;
	QPointer<QCheckBox> stageToggle;
	QPointer<QAction> editMenuAction;
	QPointer<QAction> fileMenuAction;
	QPointer<QAction> helpMenuAction;
	QPointer<QAction> fitToScreenAction;
	QPointer<QAction> screensAction;
	QPointer<QAction> soundAction;
	QPointer<QAction> bibleAction;
	QPointer<QAction> transmissionAction;
	QPointer<QAction> opbsUpdateAction;
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
	QString bibleBackgroundPath;
	QString activeBibleText;
	QString activeBibleReference;
	QString selectedMonitorName;
	QString audioDeviceName;
	QString audioDeviceId;
	QString selectedEffect;
	QString selectedCameraId;
	QString selectedCameraName;
	QString selectedTransmissionInputId;
	QString selectedTransmissionInputName;
	QString combinedBackgroundType = "color";
	QString combinedBackgroundColor = "#000000";
	QString combinedBackgroundPath;
	StreamDestination streamDestinations[2];
	OBSOutputAutoRelease secondaryStreamOutput;
	OBSServiceAutoRelease secondaryStreamService;
	int selectedMonitor = -1;
	int mediaVolume = 100;
	int outputVolume = 100;
	int outputGain = 0;
	int bibleFontSize = 96;
	int streamVideoBitrate = 6000;
	int streamAudioBitrate = 160;
	int transmissionTransitionDuration = 600;
	double transmissionPresenterDb = 0.0;
	double transmissionInputDb = 0.0;
	double combinedCameraX = 3.5;
	double combinedCameraY = 31.0;
	double combinedCameraWidth = 35.0;
	double combinedCameraHeight = 35.0;
	double combinedPresenterX = 41.0;
	double combinedPresenterY = 19.0;
	double combinedPresenterWidth = 57.0;
	double combinedPresenterHeight = 57.0;
	QString recordingPath;
	TransmissionView transmissionView = TransmissionView::Presenter;
	bool initialized = false;
	bool stageShowing = false;
	bool stageActive = false;
	bool stageEnabled = false;
	bool loopCurrent = false;
	bool fitContentToScreen = false;
	bool bibleBackgroundLoop = false;
	bool timelineDragging = false;
	bool restoring = false;
	bool secondaryStreamStarting = false;
	bool combinedBackgroundLoop = true;
	bool cameraEnabled = true;
	bool transmissionPresenterMuted = false;
	bool transmissionInputMuted = false;
	bool transmissionPresenterAudioAttached = false;
	qint64 seekGuardUntil = 0;
	int64_t cachedDuration = 0;
	int pendingSeekValue = -1;

	static void RenderPreview(void *data, uint32_t cx, uint32_t cy);
	static void RenderTransmissionPreview(void *data, uint32_t cx, uint32_t cy);
	static void AudioMeterUpdated(void *data, const float magnitude[], const float peak[], const float inputPeak[]);
	static void MediaStarted(void *data, calldata_t *calldata);
	static void PresenterAudioCaptured(void *data, obs_source_t *source, const struct audio_data *audio, bool muted);
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
	void ShowTransmissionDialog();
	void ApplyTransmissionView(TransmissionView view, bool save = true);
	void UpdateTransmissionItemBounds();
	void ApplyCombinedBackground();
	obs_source_t *TransmissionSceneSource(TransmissionView view) const;
	void RefreshCameraSource();
	void SetCameraEnabled(bool enabled);
	void AttachTransmissionPresenterAudio();
	void DetachTransmissionPresenterAudio();
	void RefreshTransmissionAudioInput();
	OBSSource CreateTransmissionAudioInput(const QString &deviceId, const QString &sourceName) const;
	void ApplyTransmissionAudioMix();
	void ToggleStreaming();
	void ToggleRecording();
	void StartSecondaryStream();
	void StopSecondaryStream();
	OBSServiceAutoRelease CreateStreamService(const StreamDestination &destination, const char *name) const;
	void ApplyPrimaryStreamService();
	void UpdateTransmissionButtons();
	void LaunchOpbsUpdater(bool silent = false);
	void ImportPresentation(bool pdf);
	void ReplacePresentationSlides(const QString &temporaryDirectory, int slideCount);
	void ClearPresentationEntries();
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
	QString PresentationsDirectoryPath() const;
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
