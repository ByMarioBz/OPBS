/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#include "PresenterPanel.hpp"

#include "OBSBasic.hpp"
#include "OBSProjector.hpp"
#include "OBSQTDisplay.hpp"

#include <OBSApp.hpp>
#include <components/Multiview.hpp>
#include <utility/ThumbnailManager.hpp>
#include <utility/ThumbnailView.hpp>
#include <utility/display-helpers.hpp>

#include <obs-audio-controls.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>

#include "moc_PresenterPanel.cpp"

namespace {
constexpr int kThumbnailWidth = 256;
constexpr int kThumbnailHeight = 144;
const QStringList imageExtensions = {"bmp", "gif", "jpeg", "jpg", "png", "tga", "webp"};

class PresenterMediaList final : public QListWidget {
public:
	std::function<void(const QStringList &)> filesDropped;
	std::function<void()> orderChanged;

	explicit PresenterMediaList(QWidget *parent) : QListWidget(parent)
	{
		setAcceptDrops(true);
		setDragEnabled(true);
		setDropIndicatorShown(true);
		setDragDropMode(QAbstractItemView::DragDrop);
		setDefaultDropAction(Qt::MoveAction);
	}

protected:
	void dragEnterEvent(QDragEnterEvent *event) override
	{
		if (event->mimeData()->hasUrls()) {
			event->acceptProposedAction();
			return;
		}
		QListWidget::dragEnterEvent(event);
	}

	void dragMoveEvent(QDragMoveEvent *event) override
	{
		if (event->mimeData()->hasUrls()) {
			event->acceptProposedAction();
			return;
		}
		QListWidget::dragMoveEvent(event);
	}

	void dropEvent(QDropEvent *event) override
	{
		if (event->mimeData()->hasUrls()) {
			QStringList paths;
			for (const QUrl &url : event->mimeData()->urls()) {
				if (url.isLocalFile())
					paths.push_back(url.toLocalFile());
			}
			if (!paths.isEmpty() && filesDropped)
				filesDropped(paths);
			event->acceptProposedAction();
			return;
		}
		QListWidget::dropEvent(event);
		if (orderChanged)
			orderChanged();
	}
};

struct AudioDevice {
	QString name;
	QString id;
};

bool AddAudioDevice(void *data, const char *name, const char *id)
{
	auto *devices = static_cast<QList<AudioDevice> *>(data);
	devices->push_back({QString::fromUtf8(name), QString::fromUtf8(id)});
	return true;
}

QString FormatTime(int64_t milliseconds)
{
	const qint64 totalSeconds = std::max<int64_t>(milliseconds, 0) / 1000;
	const qint64 hours = totalSeconds / 3600;
	const qint64 minutes = (totalSeconds / 60) % 60;
	const qint64 seconds = totalSeconds % 60;
	return hours > 0 ? QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'))
			 : QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
} // namespace

PresenterPanel::PresenterPanel(OBSBasic *main_) : QWidget(main_), main(main_)
{
	setObjectName("presenterPanel");
	setAcceptDrops(true);
	BuildInterface();
}

PresenterPanel::~PresenterPanel()
{
	Shutdown();
}

void PresenterPanel::BuildInterface()
{
	setStyleSheet(R"(
		#presenterPanel { background: #111318; color: #f4f6fb; }
		#presenterHeader { background: #181b22; border-bottom: 1px solid #2a2e38; }
		#presenterTitle { font-size: 21px; font-weight: 700; }
		#presenterSubtitle, #presenterMediaCount, #presenterScreenStatus, #presenterTime { color: #9ca4b4; }
		#presenterLive { color: #ff5c68; font-size: 11px; font-weight: 700; }
		#presenterPreviewFrame, #presenterLibrary { background: #181b22; border: 1px solid #2a2e38; border-radius: 10px; }
		#presenterCurrent { color: #d7dbea; font-weight: 600; }
		#presenterEmpty { color: #858d9d; font-size: 15px; }
		QToolButton#presenterImport { background: #6d5dfc; color: white; border: 0; border-radius: 7px; padding: 9px 15px; font-weight: 700; }
		QToolButton#presenterImport:hover { background: #8072ff; }
		QListWidget#presenterMediaList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterMediaList::item { background: #20242d; color: #eef0f6; border: 1px solid #303643; border-radius: 9px; padding: 8px; }
		QListWidget#presenterMediaList::item:hover { border-color: #6d5dfc; background: #252a35; }
		QListWidget#presenterMediaList::item:selected { border: 2px solid #7b6cff; background: #29263d; }
		QProgressBar#presenterMeter { background: #252a33; border: 0; border-radius: 3px; max-height: 7px; }
		QProgressBar#presenterMeter::chunk { background: #42d17c; border-radius: 3px; }
		QSlider::groove:horizontal { height: 6px; background: #303643; border-radius: 3px; }
		QSlider::handle:horizontal { background: #7b6cff; width: 15px; margin: -5px 0; border-radius: 7px; }
		QCheckBox#presenterStageToggle { spacing: 7px; color: #d7dbea; }
		QToolButton#presenterTransport { background: #252a35; color: #f4f6fb; border: 1px solid #373e4c;
			border-radius: 7px; min-width: 38px; min-height: 30px; font-size: 16px; font-weight: 700; }
		QToolButton#presenterTransport:hover { background: #313747; border-color: #7b6cff; }
		QToolButton#presenterTransport:disabled { color: #666d7b; background: #20242c; }
	)" );

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);
	auto *header = new QFrame(this);
	header->setObjectName("presenterHeader");
	auto *headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(22, 14, 22, 14);
	auto *headings = new QVBoxLayout();
	auto *title = new QLabel(tr("Presentador multimedia"), header);
	title->setObjectName("presenterTitle");
	auto *subtitle = new QLabel(tr("Selecciona un archivo para enviarlo al escenario"), header);
	subtitle->setObjectName("presenterSubtitle");
	headings->addWidget(title);
	headings->addWidget(subtitle);
	headerLayout->addLayout(headings);
	headerLayout->addStretch();
	stageStatus = new QLabel(tr("Escenario: sin pantalla"), header);
	stageStatus->setObjectName("presenterScreenStatus");
	headerLayout->addWidget(stageStatus);
	stageToggle = new QCheckBox(tr("Salida"), header);
	stageToggle->setObjectName("presenterStageToggle");
	stageToggle->setToolTip(tr("Activar o desactivar la salida del escenario"));
	connect(stageToggle, &QCheckBox::toggled, this, [this](bool enabled) { SetStageEnabled(enabled); });
	headerLayout->addWidget(stageToggle);
	auto *importButton = new QToolButton(header);
	importButton->setObjectName("presenterImport");
	importButton->setText(tr("+ Importar contenido"));
	connect(importButton, &QToolButton::clicked, this, &PresenterPanel::ImportMedia);
	headerLayout->addWidget(importButton);
	root->addWidget(header);

	auto *splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setChildrenCollapsible(false);
	splitter->setHandleWidth(8);
	auto *previewFrame = new QFrame(splitter);
	previewFrame->setObjectName("presenterPreviewFrame");
	auto *previewLayout = new QVBoxLayout(previewFrame);
	previewLayout->setContentsMargins(16, 16, 16, 16);
	auto *live = new QLabel(tr("●  VISTA PREVIA EN VIVO"), previewFrame);
	live->setObjectName("presenterLive");
	previewLayout->addWidget(live);
	preview = new OBSQTDisplay(previewFrame);
	preview->setMinimumSize(320, 180);
	preview->SetDisplayBackgroundColor(QColor("#08090c"));
	previewLayout->addWidget(preview, 1);
	currentMedia = new QLabel(tr("Ningún contenido seleccionado"), previewFrame);
	currentMedia->setObjectName("presenterCurrent");
	currentMedia->setAlignment(Qt::AlignCenter);
	previewLayout->addWidget(currentMedia);

	auto *timelineRow = new QHBoxLayout();
	timelineSlider = new QSlider(Qt::Horizontal, previewFrame);
	timelineSlider->setRange(0, 1000);
	timelineSlider->setEnabled(false);
	timeLabel = new QLabel("0:00 / 0:00", previewFrame);
	timeLabel->setObjectName("presenterTime");
	timelineRow->addWidget(timelineSlider, 1);
	timelineRow->addWidget(timeLabel);
	previewLayout->addLayout(timelineRow);
	connect(timelineSlider, &QSlider::sliderPressed, this, [this]() { timelineDragging = true; });
	connect(timelineSlider, &QSlider::sliderReleased, this, [this]() {
		if (activeSource) {
			const int64_t duration = obs_source_media_get_duration(activeSource);
			if (duration > 0)
				obs_source_media_set_time(activeSource, duration * timelineSlider->value() / 1000);
		}
		timelineDragging = false;
	});

	auto *transportRow = new QHBoxLayout();
	transportRow->addStretch();
	auto makeTransportButton = [previewFrame, transportRow](const QString &text, const QString &tip) {
		auto *button = new QToolButton(previewFrame);
		button->setObjectName("presenterTransport");
		button->setText(text);
		button->setToolTip(tip);
		transportRow->addWidget(button);
		return button;
	};
	auto *previousButton = makeTransportButton("⏮", tr("Anterior (tecla multimedia anterior)"));
	playPauseButton = makeTransportButton("▶", tr("Reproducir / pausar (tecla multimedia)"));
	auto *stopButton = makeTransportButton("■", tr("Detener (tecla multimedia detener)"));
	auto *nextButton = makeTransportButton("⏭", tr("Siguiente (tecla multimedia siguiente)"));
	transportRow->addStretch();
	previewLayout->addLayout(transportRow);
	connect(previousButton, &QToolButton::clicked, this, &PresenterPanel::PreviousMedia);
	connect(playPauseButton, &QToolButton::clicked, this, &PresenterPanel::TogglePlayPause);
	connect(stopButton, &QToolButton::clicked, this, &PresenterPanel::StopMedia);
	connect(nextButton, &QToolButton::clicked, this, &PresenterPanel::NextMedia);

	auto addMediaShortcut = [this](Qt::Key key, auto handler) {
		auto *shortcut = new QShortcut(QKeySequence(key), main);
		shortcut->setContext(Qt::ApplicationShortcut);
		connect(shortcut, &QShortcut::activated, this, handler);
	};
	addMediaShortcut(Qt::Key_MediaPlay, [this]() { PlayMedia(); });
	addMediaShortcut(Qt::Key_MediaPause, [this]() { PauseMedia(); });
	addMediaShortcut(Qt::Key_MediaTogglePlayPause, [this]() { TogglePlayPause(); });
	addMediaShortcut(Qt::Key_MediaStop, [this]() { StopMedia(); });
	addMediaShortcut(Qt::Key_MediaPrevious, [this]() { PreviousMedia(); });
	addMediaShortcut(Qt::Key_MediaNext, [this]() { NextMedia(); });

	auto *audioRow = new QHBoxLayout();
	auto *audioLabel = new QLabel(tr("Audio"), previewFrame);
	audioLabel->setMinimumWidth(44);
	auto *meters = new QVBoxLayout();
	meters->setSpacing(3);
	meterLeft = new QProgressBar(previewFrame);
	meterRight = new QProgressBar(previewFrame);
	for (QProgressBar *meter : {meterLeft.data(), meterRight.data()}) {
		meter->setObjectName("presenterMeter");
		meter->setRange(0, 100);
		meter->setValue(0);
		meter->setTextVisible(false);
		meters->addWidget(meter);
	}
	mediaVolumeSlider = new QSlider(Qt::Horizontal, previewFrame);
	mediaVolumeSlider->setRange(0, 100);
	mediaVolumeSlider->setValue(100);
	mediaVolumeSlider->setToolTip(tr("Volumen del contenido"));
	audioRow->addWidget(audioLabel);
	audioRow->addLayout(meters, 1);
	audioRow->addWidget(new QLabel(tr("Volumen"), previewFrame));
	audioRow->addWidget(mediaVolumeSlider, 1);
	previewLayout->addLayout(audioRow);
	connect(mediaVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
		mediaVolume = value;
		ApplyAudioSettings();
		if (!restoring)
			SaveSettings();
	});

	auto *library = new QFrame(splitter);
	library->setObjectName("presenterLibrary");
	auto *libraryLayout = new QVBoxLayout(library);
	libraryLayout->setContentsMargins(16, 16, 16, 16);
	auto *libraryHead = new QHBoxLayout();
	auto *libraryTitle = new QLabel(tr("Biblioteca"), library);
	libraryTitle->setObjectName("presenterTitle");
	mediaCount = new QLabel(tr("0 archivos"), library);
	mediaCount->setObjectName("presenterMediaCount");
	libraryHead->addWidget(libraryTitle);
	libraryHead->addStretch();
	libraryHead->addWidget(mediaCount);
	libraryLayout->addLayout(libraryHead);
	emptyState = new QLabel(tr("Arrastra aquí imágenes, videos o audio para comenzar"), library);
	emptyState->setObjectName("presenterEmpty");
	emptyState->setAlignment(Qt::AlignCenter);
	emptyState->setMinimumHeight(220);
	libraryLayout->addWidget(emptyState, 1);
	auto *list = new PresenterMediaList(library);
	mediaList = list;
	list->setObjectName("presenterMediaList");
	list->setViewMode(QListView::IconMode);
	list->setResizeMode(QListView::Adjust);
	list->setMovement(QListView::Snap);
	list->setWrapping(true);
	list->setIconSize(QSize(kThumbnailWidth, kThumbnailHeight));
	list->setGridSize(QSize(kThumbnailWidth + 24, kThumbnailHeight + 58));
	list->setSpacing(8);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->hide();
	list->filesDropped = [this](const QStringList &paths) { ImportPaths(paths); };
	list->orderChanged = [this]() { ReorderEntriesFromList(); };
	connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
		for (const auto &entry : entries) {
			if (entry->item == item) {
				ActivateMedia(entry.get());
				break;
			}
		}
	});
	libraryLayout->addWidget(list, 1);

	splitter->addWidget(previewFrame);
	splitter->addWidget(library);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({600, 600});
	root->addWidget(splitter, 1);

	timelineTimer = new QTimer(this);
	timelineTimer->setInterval(250);
	connect(timelineTimer, &QTimer::timeout, this, &PresenterPanel::RefreshTimeline);
}

void PresenterPanel::BuildTopMenu()
{
	screensAction = main->menuBar()->addAction(tr("Pantallas"));
	soundAction = main->menuBar()->addAction(tr("Sonido"));
	connect(screensAction, &QAction::triggered, this, &PresenterPanel::ShowScreensDialog);
	connect(soundAction, &QAction::triggered, this, &PresenterPanel::ShowSoundDialog);
	for (QAction *action : main->menuBar()->actions())
		action->setVisible(action == screensAction || action == soundAction);

	connect(qApp, &QGuiApplication::screenAdded, this, [this]() {
		ResolveSelectedMonitor();
		UpdateStageStatus();
	});
	connect(qApp, &QGuiApplication::screenRemoved, this, [this]() {
		ResolveSelectedMonitor();
		if (stageEnabled)
			SetStageEnabled(true, false);
		UpdateStageStatus();
	});
}

void PresenterPanel::Initialize()
{
	if (initialized)
		return;
	OBSSceneAutoRelease scene = obs_scene_create_private("Presenter Stage");
	stageScene = scene.Get();
	if (!stageScene) {
		QMessageBox::critical(this, tr("Presentador multimedia"), tr("No fue posible crear el escenario de reproducción."));
		return;
	}
	BuildTopMenu();
	originalCentralWidget = main->takeCentralWidget();
	if (originalCentralWidget) {
		originalCentralWidget->hide();
		originalCentralWidget->setParent(main);
	}
	main->setCentralWidget(this);
	main->setWindowTitle(tr("Presentador multimedia — basado en OBS Studio"));
	main->statusBar()->hide();
	// Esta variante no utiliza el asistente de transmisión/grabación de OBS.
	config_set_bool(App()->GetUserConfig(), "General", "FirstRun", true);
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
	QTimer::singleShot(0, this, [this]() {
		for (QDockWidget *dock : main->findChildren<QDockWidget *>())
			dock->hide();
	});
	connect(preview, &OBSQTDisplay::DisplayCreated, this, [this](OBSQTDisplay *display) {
		obs_display_add_draw_callback(display->GetDisplay(), PresenterPanel::RenderPreview, this);
	});
	obs_source_t *stageSource = obs_scene_get_source(stageScene);
	obs_source_inc_showing(stageSource);
	stageShowing = true;
	// La escena privada necesita estar activa para que OBS procese el audio de sus fuentes.
	obs_source_inc_active(stageSource);
	stageActive = true;
	audioMeter = obs_volmeter_create(OBS_FADER_LOG);
	if (audioMeter)
		obs_volmeter_add_callback(audioMeter, PresenterPanel::AudioMeterUpdated, this);
	initialized = true;
	LoadSettings();
	timelineTimer->start();
}

void PresenterPanel::Shutdown()
{
	if (!initialized && !stageScene)
		return;
	SaveSettings();
	if (timelineTimer)
		timelineTimer->stop();
	if (stageProjector) {
		main->DeleteProjector(stageProjector);
		stageProjector = nullptr;
	}
	if (preview && preview->GetDisplay())
		obs_display_remove_draw_callback(preview->GetDisplay(), PresenterPanel::RenderPreview, this);
	if (audioMeter) {
		obs_volmeter_detach_source(audioMeter);
		obs_volmeter_remove_callback(audioMeter, PresenterPanel::AudioMeterUpdated, this);
		obs_volmeter_destroy(audioMeter);
		audioMeter = nullptr;
	}
	DetachAudioFilters();
	activeItem = nullptr;
	if (activeSource) {
		obs_source_media_stop(activeSource);
		obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_NONE);
	}
	activeSource = nullptr;
	for (const auto &entry : entries) {
		if (entry->thumbnailPrepareTimer)
			entry->thumbnailPrepareTimer->stop();
		if (entry->thumbnailReleaseTimer)
			entry->thumbnailReleaseTimer->stop();
		if (entry->thumbnailShowing && entry->source)
			obs_source_dec_showing(entry->source);
	}
	entries.clear();
	if (stageScene && stageActive) {
		obs_source_dec_active(obs_scene_get_source(stageScene));
		stageActive = false;
	}
	if (stageScene && stageShowing) {
		obs_source_dec_showing(obs_scene_get_source(stageScene));
		stageShowing = false;
	}
	stageScene = nullptr;
	initialized = false;
}

void PresenterPanel::ImportMedia()
{
	const QString filter = tr("Contenido multimedia (*.bmp *.gif *.jpeg *.jpg *.png *.tga *.webp *.mp4 *.m4v *.mov *.mkv *.avi *.webm *.wmv *.mpeg *.mpg *.mp3 *.wav *.m4a *.aac *.flac *.ogg);;Todos los archivos (*.*)");
	ImportPaths(QFileDialog::getOpenFileNames(this, tr("Importar contenido multimedia"), QString(), filter));
}

void PresenterPanel::ImportPaths(const QStringList &paths)
{
	for (const QString &path : paths)
		AddMediaFile(path, false);
	SaveSettings();
}

void PresenterPanel::AddMediaFile(const QString &path, bool save)
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isFile())
		return;
	for (const auto &existing : entries) {
		if (QFileInfo(existing->path).absoluteFilePath() == info.absoluteFilePath())
			return;
	}
	const bool isImage = imageExtensions.contains(info.suffix().toLower());
	OBSDataAutoRelease settings = obs_data_create();
	const char *sourceType = isImage ? "image_source" : "ffmpeg_source";
	if (isImage) {
		obs_data_set_string(settings, "file", path.toUtf8().constData());
	} else {
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_string(settings, "local_file", path.toUtf8().constData());
		obs_data_set_bool(settings, "restart_on_activate", true);
		obs_data_set_bool(settings, "close_when_inactive", true);
		obs_data_set_bool(settings, "clear_on_media_end", true);
	}
	sourceType = obs_get_latest_input_type_id(sourceType);
	if (!sourceType)
		return;
	auto entry = std::make_unique<MediaEntry>();
	entry->path = info.absoluteFilePath();
	OBSSourceAutoRelease source = obs_source_create_private(sourceType, info.fileName().toUtf8().constData(), settings);
	entry->source = source.Get();
	if (!entry->source)
		return;
	entry->item = new QListWidgetItem(info.fileName(), mediaList);
	entry->item->setData(Qt::UserRole, entry->path);
	entry->item->setToolTip(entry->path);
	entry->item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	MediaEntry *entryPtr = entry.get();
	QPixmap thumbnail;
	if (isImage) {
		QImageReader reader(path);
		reader.setAutoTransform(true);
		reader.setScaledSize(QSize(kThumbnailWidth, kThumbnailHeight));
		thumbnail = QPixmap::fromImage(reader.read());
	}
	if (thumbnail.isNull())
		thumbnail = PlaceholderForSource(entry->source);
	SetCardThumbnail(entryPtr, thumbnail);
	if (!isImage) {
		entry->thumbnailView = App()->thumbnails()->createView(mediaList, entry->source);
		connect(entry->thumbnailView, &ThumbnailView::updated, mediaList, [this, entryPtr](const QPixmap &pixmap) { SetCardThumbnail(entryPtr, pixmap); });
		obs_source_inc_showing(entry->source);
		entry->thumbnailShowing = true;
		entry->thumbnailPrepareTimer = new QTimer(this);
		entry->thumbnailPrepareTimer->setSingleShot(true);
		connect(entry->thumbnailPrepareTimer, &QTimer::timeout, this, [entryPtr]() {
			obs_source_media_set_time(entryPtr->source, 0);
			obs_source_media_play_pause(entryPtr->source, true);
			if (entryPtr->thumbnailView)
				entryPtr->thumbnailView->requestUpdate();
		});
		entry->thumbnailPrepareTimer->start(350);
		entry->thumbnailReleaseTimer = new QTimer(this);
		entry->thumbnailReleaseTimer->setSingleShot(true);
		connect(entry->thumbnailReleaseTimer, &QTimer::timeout, this, [entryPtr]() {
			if (entryPtr->thumbnailShowing) {
				obs_source_dec_showing(entryPtr->source);
				entryPtr->thumbnailShowing = false;
			}
		});
		entry->thumbnailReleaseTimer->start(1800);
	}
	entries.emplace_back(std::move(entry));
	emptyState->hide();
	mediaList->show();
	mediaCount->setText(tr("%1 archivos").arg(static_cast<qulonglong>(entries.size())));
	if (save)
		SaveSettings();
}

QPixmap PresenterPanel::PlaceholderForSource(obs_source_t *source) const
{
	QPixmap result(kThumbnailWidth, kThumbnailHeight);
	result.fill(QColor("#111318"));
	const QPixmap icon = main->GetSourceIcon(obs_source_get_id(source)).pixmap(QSize(64, 64));
	QPainter painter(&result);
	painter.drawPixmap((result.width() - icon.width()) / 2, (result.height() - icon.height()) / 2, icon);
	return result;
}

void PresenterPanel::SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap)
{
	if (entry && entry->item && !pixmap.isNull())
		entry->item->setIcon(QIcon(pixmap.scaled(kThumbnailWidth, kThumbnailHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void PresenterPanel::ActivateMedia(MediaEntry *entry)
{
	if (!entry || !entry->source || !stageScene)
		return;
	if (audioMeter)
		obs_volmeter_detach_source(audioMeter);
	DetachAudioFilters();
	if (activeSource) {
		obs_source_media_stop(activeSource);
		obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_NONE);
	}
	if (activeItem) {
		obs_sceneitem_remove(activeItem);
		activeItem = nullptr;
	}
	activeSource = entry->source;
	obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
	activeItem = obs_scene_add(stageScene, activeSource);
	if (!activeItem)
		return;
	obs_sceneitem_set_bounds_type(activeItem, OBS_BOUNDS_SCALE_INNER);
	obs_sceneitem_set_bounds_alignment(activeItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_alignment(activeItem, OBS_ALIGN_CENTER);
	const struct vec2 size = {(float)obs_source_get_width(obs_scene_get_source(stageScene)), (float)obs_source_get_height(obs_scene_get_source(stageScene))};
	const struct vec2 position = {size.x / 2.0f, size.y / 2.0f};
	obs_sceneitem_set_bounds(activeItem, &size);
	obs_sceneitem_set_pos(activeItem, &position);
	ApplyAudioSettings();
	if (audioMeter)
		obs_volmeter_attach_source(audioMeter, activeSource);
	obs_source_media_restart(activeSource);
	mediaList->setCurrentItem(entry->item);
	currentMedia->setText(QFileInfo(entry->path).fileName());
	RefreshTimeline();
}

void PresenterPanel::ApplyAudioSettings()
{
	if (!activeSource)
		return;
	obs_source_set_volume(activeSource, (mediaVolume / 100.0f) * (outputVolume / 100.0f));
	DetachAudioFilters();
	if (outputGain != 0) {
		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_double(settings, "db", outputGain);
		OBSSourceAutoRelease filter = obs_source_create_private("gain_filter", "Presenter Output Gain", settings);
		gainFilter = filter.Get();
		if (gainFilter)
			obs_source_filter_add(activeSource, gainFilter);
	}
	if (selectedEffect == "compressor" || selectedEffect == "both") {
		OBSSourceAutoRelease filter = obs_source_create_private("compressor_filter", "Presenter Compressor", nullptr);
		compressorFilter = filter.Get();
		if (compressorFilter)
			obs_source_filter_add(activeSource, compressorFilter);
	}
	if (selectedEffect == "limiter" || selectedEffect == "both") {
		OBSSourceAutoRelease filter = obs_source_create_private("limiter_filter", "Presenter Limiter", nullptr);
		limiterFilter = filter.Get();
		if (limiterFilter)
			obs_source_filter_add(activeSource, limiterFilter);
	}
}

void PresenterPanel::DetachAudioFilters()
{
	if (activeSource) {
		if (gainFilter)
			obs_source_filter_remove(activeSource, gainFilter);
		if (compressorFilter)
			obs_source_filter_remove(activeSource, compressorFilter);
		if (limiterFilter)
			obs_source_filter_remove(activeSource, limiterFilter);
	}
	gainFilter = nullptr;
	compressorFilter = nullptr;
	limiterFilter = nullptr;
}

void PresenterPanel::RefreshTimeline()
{
	if (!activeSource) {
		timelineSlider->setEnabled(false);
		timeLabel->setText("0:00 / 0:00");
		if (playPauseButton)
			playPauseButton->setText("▶");
		return;
	}
	const int64_t duration = obs_source_media_get_duration(activeSource);
	const int64_t time = obs_source_media_get_time(activeSource);
	const obs_media_state state = obs_source_media_get_state(activeSource);
	if (playPauseButton)
		playPauseButton->setText(state == OBS_MEDIA_STATE_PLAYING ? "⏸" : "▶");
	timelineSlider->setEnabled(duration > 0);
	if (!timelineDragging && duration > 0)
		timelineSlider->setValue(int(std::clamp<int64_t>(time * 1000 / duration, 0, 1000)));
	timeLabel->setText(QString("%1 / %2").arg(FormatTime(time), FormatTime(duration)));
}

void PresenterPanel::TogglePlayPause()
{
	if (!activeSource)
		return;
	if (obs_source_media_get_state(activeSource) == OBS_MEDIA_STATE_PLAYING)
		PauseMedia();
	else
		PlayMedia();
}

void PresenterPanel::PlayMedia()
{
	if (!activeSource)
		return;
	const obs_media_state state = obs_source_media_get_state(activeSource);
	if (state == OBS_MEDIA_STATE_STOPPED || state == OBS_MEDIA_STATE_ENDED || state == OBS_MEDIA_STATE_NONE)
		obs_source_media_restart(activeSource);
	else
		obs_source_media_play_pause(activeSource, false);
	RefreshTimeline();
}

void PresenterPanel::PauseMedia()
{
	if (activeSource) {
		obs_source_media_play_pause(activeSource, true);
		RefreshTimeline();
	}
}

void PresenterPanel::StopMedia()
{
	if (!activeSource)
		return;
	obs_source_media_stop(activeSource);
	timelineSlider->setValue(0);
	meterLeft->setValue(0);
	meterRight->setValue(0);
	RefreshTimeline();
}

void PresenterPanel::NextMedia()
{
	if (!mediaList || mediaList->count() == 0)
		return;
	const int current = mediaList->currentRow();
	const int next = current < 0 ? 0 : (current + 1) % mediaList->count();
	QListWidgetItem *item = mediaList->item(next);
	for (const auto &entry : entries) {
		if (entry->item == item) {
			ActivateMedia(entry.get());
			return;
		}
	}
}

void PresenterPanel::PreviousMedia()
{
	if (!mediaList || mediaList->count() == 0)
		return;
	const int current = mediaList->currentRow();
	const int previous = current < 0 ? mediaList->count() - 1
					 : (current - 1 + mediaList->count()) % mediaList->count();
	QListWidgetItem *item = mediaList->item(previous);
	for (const auto &entry : entries) {
		if (entry->item == item) {
			ActivateMedia(entry.get());
			return;
		}
	}
}

void PresenterPanel::AudioMeterUpdated(void *data, const float[], const float peak[], const float[])
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel)
		return;
	const auto level = [](float db) { return std::clamp(int((db + 60.0f) * 100.0f / 60.0f), 0, 100); };
	const int left = level(peak[0]);
	const int right = level(peak[1]);
	QMetaObject::invokeMethod(panel, [panel, left, right]() {
		if (panel->meterLeft)
			panel->meterLeft->setValue(left);
		if (panel->meterRight)
			panel->meterRight->setValue(right);
	}, Qt::QueuedConnection);
}

void PresenterPanel::ShowScreensDialog()
{
	QDialog dialog(main);
	dialog.setWindowTitle(tr("Configuración de pantallas"));
	dialog.setMinimumWidth(480);
	auto *layout = new QVBoxLayout(&dialog);
	auto *title = new QLabel(tr("Escenario"), &dialog);
	title->setObjectName("presenterTitle");
	layout->addWidget(title);
	layout->addWidget(new QLabel(tr("Elige la pantalla conectada donde se proyectará el contenido a tamaño completo."), &dialog));
	auto *combo = new QComboBox(&dialog);
	const auto screens = QGuiApplication::screens();
	const auto descriptions = OBSBasic::GetProjectorMenuMonitorsFormatted();
	for (int i = 0; i < screens.size(); ++i) {
		combo->addItem(descriptions.value(i, screens[i]->name()), screens[i]->name());
		if (screens[i]->name() == selectedMonitorName)
			combo->setCurrentIndex(i);
	}
	layout->addWidget(combo);
	auto *enabled = new QCheckBox(tr("Activar salida del escenario"), &dialog);
	enabled->setChecked(stageEnabled);
	layout->addWidget(enabled);
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (dialog.exec() != QDialog::Accepted)
		return;
	selectedMonitorName = combo->currentData().toString();
	ResolveSelectedMonitor();
	SetStageEnabled(enabled->isChecked(), false);
	SaveSettings();
}

void PresenterPanel::ShowSoundDialog()
{
	QList<AudioDevice> devices;
	obs_enum_audio_monitoring_devices(AddAudioDevice, &devices);
	QDialog dialog(main);
	dialog.setWindowTitle(tr("Configuración de sonido"));
	dialog.setMinimumWidth(500);
	auto *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel(tr("Altavoces / monitor de salida"), &dialog));
	auto *deviceCombo = new QComboBox(&dialog);
	for (const AudioDevice &device : devices) {
		deviceCombo->addItem(device.name, device.id);
		if (device.id == audioDeviceId)
			deviceCombo->setCurrentIndex(deviceCombo->count() - 1);
	}
	layout->addWidget(deviceCombo);
	layout->addWidget(new QLabel(tr("Volumen de salida"), &dialog));
	auto *volume = new QSlider(Qt::Horizontal, &dialog);
	volume->setRange(0, 100);
	volume->setValue(outputVolume);
	layout->addWidget(volume);
	auto *gainLabel = new QLabel(tr("Ganancia de salida: %1 dB").arg(outputGain), &dialog);
	auto *gain = new QSlider(Qt::Horizontal, &dialog);
	gain->setRange(-30, 30);
	gain->setValue(outputGain);
	connect(gain, &QSlider::valueChanged, gainLabel, [gainLabel](int value) { gainLabel->setText(QObject::tr("Ganancia de salida: %1 dB").arg(value)); });
	layout->addWidget(gainLabel);
	layout->addWidget(gain);
	layout->addWidget(new QLabel(tr("Efecto de salida"), &dialog));
	auto *effect = new QComboBox(&dialog);
	effect->addItem(tr("Ninguno"), "none");
	effect->addItem(tr("Compresor"), "compressor");
	effect->addItem(tr("Limitador"), "limiter");
	effect->addItem(tr("Compresor + limitador"), "both");
	const int effectIndex = effect->findData(selectedEffect);
	effect->setCurrentIndex(effectIndex >= 0 ? effectIndex : 0);
	layout->addWidget(effect);
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (dialog.exec() != QDialog::Accepted)
		return;
	audioDeviceName = deviceCombo->currentText();
	audioDeviceId = deviceCombo->currentData().toString();
	if (!audioDeviceId.isEmpty())
		obs_set_audio_monitoring_device(audioDeviceName.toUtf8().constData(), audioDeviceId.toUtf8().constData());
	outputVolume = volume->value();
	outputGain = gain->value();
	selectedEffect = effect->currentData().toString();
	ApplyAudioSettings();
	SaveSettings();
}

void PresenterPanel::ResolveSelectedMonitor()
{
	selectedMonitor = -1;
	const auto screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); ++i) {
		if (screens[i]->name() == selectedMonitorName) {
			selectedMonitor = i;
			break;
		}
	}
}

void PresenterPanel::SetStageEnabled(bool enabled, bool save)
{
	if (stageProjector) {
		main->DeleteProjector(stageProjector);
		stageProjector = nullptr;
	}
	ResolveSelectedMonitor();
	stageEnabled = enabled && selectedMonitor >= 0;
	if (stageEnabled && stageScene) {
		stageProjector = main->OpenPresenterProjector(obs_scene_get_source(stageScene), selectedMonitor);
		OBSProjector *projector = stageProjector;
		if (projector) {
			connect(projector, &QObject::destroyed, this, [this, projector]() {
				if (stageProjector.data() == projector) {
					stageProjector = nullptr;
					stageEnabled = false;
					UpdateStageStatus();
				}
			});
		}
	}
	if (stageToggle) {
		stageToggle->blockSignals(true);
		stageToggle->setChecked(stageEnabled);
		stageToggle->blockSignals(false);
	}
	UpdateStageStatus();
	if (save && !restoring)
		SaveSettings();
}

void PresenterPanel::UpdateStageStatus()
{
	QString text = tr("Escenario: sin pantalla");
	if (selectedMonitor >= 0)
		text = stageEnabled ? tr("Escenario: %1 · activo").arg(selectedMonitorName) : tr("Escenario: %1 · apagado").arg(selectedMonitorName);
	stageStatus->setText(text);
}

QString PresenterPanel::SettingsPath() const
{
	BPtr<char> path(GetAppConfigPathPtr("obs-studio/presenter.ini"));
	return QString::fromUtf8(path.Get());
}

void PresenterPanel::LoadSettings()
{
	restoring = true;
	QSettings settings(SettingsPath(), QSettings::IniFormat);
	mediaVolume = settings.value("audio/mediaVolume", 100).toInt();
	outputVolume = settings.value("audio/outputVolume", 100).toInt();
	outputGain = settings.value("audio/gain", 0).toInt();
	selectedEffect = settings.value("audio/effect", "none").toString();
	audioDeviceName = settings.value("audio/deviceName").toString();
	audioDeviceId = settings.value("audio/deviceId").toString();
	selectedMonitorName = settings.value("stage/monitorName").toString();
	stageEnabled = settings.value("stage/enabled", false).toBool();
	mediaVolumeSlider->setValue(mediaVolume);
	const QByteArray geometry = settings.value("window/geometry").toByteArray();
	if (!geometry.isEmpty())
		main->restoreGeometry(geometry);
	if (!audioDeviceId.isEmpty())
		obs_set_audio_monitoring_device(audioDeviceName.toUtf8().constData(), audioDeviceId.toUtf8().constData());
	for (const QString &path : settings.value("library/files").toStringList())
		AddMediaFile(path, false);
	ResolveSelectedMonitor();
	SetStageEnabled(stageEnabled, false);
	restoring = false;
	SaveSettings();
}

void PresenterPanel::SaveSettings()
{
	if (restoring)
		return;
	const QString path = SettingsPath();
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSettings settings(path, QSettings::IniFormat);
	QStringList files;
	if (mediaList) {
		for (int i = 0; i < mediaList->count(); ++i)
			files.push_back(mediaList->item(i)->data(Qt::UserRole).toString());
	}
	settings.setValue("library/files", files);
	settings.setValue("stage/monitorName", selectedMonitorName);
	settings.setValue("stage/enabled", stageEnabled);
	settings.setValue("audio/mediaVolume", mediaVolume);
	settings.setValue("audio/outputVolume", outputVolume);
	settings.setValue("audio/gain", outputGain);
	settings.setValue("audio/effect", selectedEffect);
	settings.setValue("audio/deviceName", audioDeviceName);
	settings.setValue("audio/deviceId", audioDeviceId);
	settings.setValue("window/geometry", main->saveGeometry());
	settings.sync();
}

void PresenterPanel::ReorderEntriesFromList()
{
	std::vector<std::unique_ptr<MediaEntry>> ordered;
	ordered.reserve(entries.size());
	for (int row = 0; row < mediaList->count(); ++row) {
		QListWidgetItem *item = mediaList->item(row);
		auto found = std::find_if(entries.begin(), entries.end(), [item](const auto &entry) { return entry->item == item; });
		if (found != entries.end()) {
			ordered.emplace_back(std::move(*found));
			entries.erase(found);
		}
	}
	for (auto &entry : entries)
		ordered.emplace_back(std::move(entry));
	entries = std::move(ordered);
	SaveSettings();
}

void PresenterPanel::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void PresenterPanel::dropEvent(QDropEvent *event)
{
	QStringList paths;
	for (const QUrl &url : event->mimeData()->urls()) {
		if (url.isLocalFile())
			paths.push_back(url.toLocalFile());
	}
	ImportPaths(paths);
	event->acceptProposedAction();
}

void PresenterPanel::RenderPreview(void *data, uint32_t cx, uint32_t cy)
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel || !panel->stageScene)
		return;
	obs_source_t *source = obs_scene_get_source(panel->stageScene);
	const uint32_t targetCX = std::max(obs_source_get_width(source), 1u);
	const uint32_t targetCY = std::max(obs_source_get_height(source), 1u);
	int x, y;
	float scale;
	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);
	const int newCX = int(scale * float(targetCX));
	const int newCY = int(scale * float(targetCY));
	startRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f, float(targetCY));
	obs_source_video_render(source);
	endRegion();
}
