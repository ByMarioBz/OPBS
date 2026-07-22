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

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QScreen>
#include <QSplitter>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <algorithm>

#include "moc_PresenterPanel.cpp"

namespace {
constexpr int kThumbnailWidth = 256;
constexpr int kThumbnailHeight = 144;

const QStringList imageExtensions = {"bmp", "gif", "jpeg", "jpg", "png", "tga", "webp"};

QFrame *CreateScreenCard(const QString &title, const QString &subtitle, QWidget *parent, QToolButton **button,
			 QLabel **status)
{
	auto *card = new QFrame(parent);
	card->setObjectName("presenterScreenCard");
	card->setMinimumWidth(190);

	auto *layout = new QVBoxLayout(card);
	layout->setContentsMargins(14, 12, 14, 12);
	layout->setSpacing(5);

	auto *top = new QHBoxLayout();
	auto *titleLabel = new QLabel(title, card);
	titleLabel->setObjectName("presenterScreenTitle");
	*button = new QToolButton(card);
	(*button)->setText("+");
	(*button)->setToolTip(QObject::tr("Elegir pantalla"));
	(*button)->setObjectName("presenterScreenAdd");
	top->addWidget(titleLabel);
	top->addStretch();
	top->addWidget(*button);
	layout->addLayout(top);

	*status = new QLabel(subtitle, card);
	(*status)->setObjectName("presenterScreenStatus");
	(*status)->setWordWrap(true);
	layout->addWidget(*status);
	return card;
}
} // namespace

PresenterPanel::PresenterPanel(OBSBasic *main_) : QWidget(main_), main(main_)
{
	setObjectName("presenterPanel");
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
		#presenterSubtitle, #presenterMediaCount, #presenterScreenStatus { color: #9ca4b4; }
		#presenterLive { color: #ff5c68; font-size: 11px; font-weight: 700; }
		#presenterPreviewFrame, #presenterLibrary { background: #181b22; border: 1px solid #2a2e38; border-radius: 10px; }
		#presenterCurrent { color: #d7dbea; font-weight: 600; }
		#presenterEmpty { color: #858d9d; font-size: 15px; }
		QToolButton#presenterImport { background: #6d5dfc; color: white; border: 0; border-radius: 7px;
			padding: 9px 15px; font-weight: 700; }
		QToolButton#presenterImport:hover { background: #8072ff; }
		QToolButton#presenterMediaCard { background: #20242d; color: #eef0f6; border: 1px solid #303643;
			border-radius: 9px; padding: 8px; font-weight: 600; }
		QToolButton#presenterMediaCard:hover { border-color: #6d5dfc; background: #252a35; }
		QToolButton#presenterMediaCard[active="true"] { border: 2px solid #7b6cff; background: #29263d; }
		#presenterScreenCard { background: #20242d; border: 1px solid #303643; border-radius: 8px; }
		#presenterScreenTitle { font-weight: 700; }
		QToolButton#presenterScreenAdd { background: #6d5dfc; color: white; border-radius: 11px;
			min-width: 22px; min-height: 22px; font-weight: 700; }
	)");

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

	auto *scroll = new QScrollArea(library);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	auto *gridHost = new QWidget(scroll);
	mediaGrid = new QGridLayout(gridHost);
	mediaGrid->setContentsMargins(2, 8, 2, 8);
	mediaGrid->setHorizontalSpacing(12);
	mediaGrid->setVerticalSpacing(12);
	mediaGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	emptyState = new QLabel(tr("Importa imágenes, videos o audio para comenzar"), gridHost);
	emptyState->setObjectName("presenterEmpty");
	emptyState->setAlignment(Qt::AlignCenter);
	emptyState->setMinimumHeight(220);
	mediaGrid->addWidget(emptyState, 0, 0, 1, 2);
	scroll->setWidget(gridHost);
	libraryLayout->addWidget(scroll, 1);

	splitter->addWidget(previewFrame);
	splitter->addWidget(library);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({600, 600});
	root->addWidget(splitter, 1);
}

void PresenterPanel::BuildScreensMenu()
{
	screensMenu = new QMenu(tr("Pantallas"), main);
	screensMenu->setObjectName("presenterScreensMenu");
	main->menuBar()->addMenu(screensMenu);

	auto *action = new QWidgetAction(screensMenu);
	auto *host = new QWidget(screensMenu);
	auto *layout = new QHBoxLayout(host);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);

	QToolButton *stageButton = nullptr;
	QLabel *stageLabel = nullptr;
	auto *stageCard = CreateScreenCard(tr("Escenario"), tr("Sin pantalla seleccionada"), host, &stageButton,
					    &stageLabel);
	menuStageStatus = stageLabel;
	monitorMenu = new QMenu(stageButton);
	stageButton->setMenu(monitorMenu);
	stageButton->setPopupMode(QToolButton::InstantPopup);
	connect(monitorMenu, &QMenu::aboutToShow, this, &PresenterPanel::RefreshMonitorMenu);
	layout->addWidget(stageCard);

	QToolButton *secondButton = nullptr;
	QLabel *secondStatus = nullptr;
	auto *secondCard = CreateScreenCard(tr("Pantalla 2"), tr("Preparada para una futura salida"), host,
					     &secondButton, &secondStatus);
	secondButton->setEnabled(false);
	layout->addWidget(secondCard);
	action->setDefaultWidget(host);
	screensMenu->addAction(action);

	connect(qApp, &QGuiApplication::screenAdded, this, [this]() { UpdateStageStatus(); });
	connect(qApp, &QGuiApplication::screenRemoved, this, [this]() {
		if (selectedMonitor >= QGuiApplication::screens().size()) {
			selectedMonitor = -1;
			stageProjector = nullptr;
		}
		UpdateStageStatus();
	});
}

void PresenterPanel::Initialize()
{
	if (initialized) {
		return;
	}

	OBSSceneAutoRelease newStageScene = obs_scene_create_private("Presenter Stage");
	stageScene = newStageScene.Get();
	if (!stageScene) {
		QMessageBox::critical(this, tr("Presentador multimedia"),
				      tr("No fue posible crear el escenario de reproducción."));
		return;
	}

	BuildScreensMenu();
	for (QAction *action : main->menuBar()->actions()) {
		if (action->menu() != screensMenu) {
			action->setVisible(false);
		}
	}
	originalCentralWidget = main->takeCentralWidget();
	if (originalCentralWidget) {
		originalCentralWidget->hide();
		originalCentralWidget->setParent(main);
	}
	main->setCentralWidget(this);
	main->setWindowTitle(tr("Presentador multimedia — basado en OBS Studio"));
	QTimer::singleShot(0, this, [this]() {
		for (QDockWidget *dock : main->findChildren<QDockWidget *>()) {
			dock->hide();
		}
	});

	connect(preview, &OBSQTDisplay::DisplayCreated, this, [this](OBSQTDisplay *display) {
		obs_display_add_draw_callback(display->GetDisplay(), PresenterPanel::RenderPreview, this);
	});

	obs_source_t *stageSource = obs_scene_get_source(stageScene);
	obs_source_inc_showing(stageSource);
	stageShowing = true;
	initialized = true;
}

void PresenterPanel::Shutdown()
{
	if (!initialized && !stageScene) {
		return;
	}

	if (stageProjector) {
		main->DeleteProjector(stageProjector);
		stageProjector = nullptr;
	}
	if (preview && preview->GetDisplay()) {
		obs_display_remove_draw_callback(preview->GetDisplay(), PresenterPanel::RenderPreview, this);
	}
	activeItem = nullptr;
	if (activeSource) {
		obs_source_media_stop(activeSource);
		obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_NONE);
	}
	activeSource = nullptr;
	for (const auto &entry : entries) {
		if (entry->thumbnailPrepareTimer) {
			entry->thumbnailPrepareTimer->stop();
			delete entry->thumbnailPrepareTimer.data();
		}
		if (entry->thumbnailReleaseTimer) {
			entry->thumbnailReleaseTimer->stop();
			delete entry->thumbnailReleaseTimer.data();
		}
		if (entry->thumbnailShowing && entry->source) {
			obs_source_dec_showing(entry->source);
			entry->thumbnailShowing = false;
		}
	}
	entries.clear();
	if (stageScene && stageShowing) {
		obs_source_dec_showing(obs_scene_get_source(stageScene));
		stageShowing = false;
	}
	stageScene = nullptr;
	initialized = false;
}

void PresenterPanel::ImportMedia()
{
	const QString filter = tr("Contenido multimedia (*.bmp *.gif *.jpeg *.jpg *.png *.tga *.webp *.mp4 *.m4v "
				  "*.mov *.mkv *.avi *.webm *.wmv *.mpeg *.mpg *.mp3 *.wav *.m4a *.aac *.flac *.ogg);;"
				  "Todos los archivos (*.*)");
	const QStringList files = QFileDialog::getOpenFileNames(this, tr("Importar contenido multimedia"), QString(),
							 filter);
	for (const QString &file : files) {
		AddMediaFile(file);
	}
}

void PresenterPanel::AddMediaFile(const QString &path)
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isFile()) {
		return;
	}
	for (const auto &existing : entries) {
		if (QFileInfo(existing->path).absoluteFilePath() == info.absoluteFilePath()) {
			return;
		}
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
	if (!sourceType) {
		QMessageBox::warning(this, tr("Archivo no compatible"),
				     tr("OBS no tiene disponible el módulo necesario para abrir %1.").arg(info.fileName()));
		return;
	}

	auto entry = std::make_unique<MediaEntry>();
	entry->path = info.absoluteFilePath();
	OBSSourceAutoRelease newSource =
		obs_source_create_private(sourceType, info.fileName().toUtf8().constData(), settings);
	entry->source = newSource.Get();
	if (!entry->source) {
		QMessageBox::warning(this, tr("Archivo no compatible"),
				     tr("No fue posible cargar %1.").arg(info.fileName()));
		return;
	}

	auto *card = new QToolButton(mediaGrid->parentWidget());
	card->setObjectName("presenterMediaCard");
	card->setProperty("active", false);
	card->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	card->setIconSize(QSize(kThumbnailWidth, kThumbnailHeight));
	card->setFixedSize(kThumbnailWidth + 20, kThumbnailHeight + 54);
	card->setText(info.fileName());
	card->setToolTip(info.absoluteFilePath());
	entry->button = card;

	MediaEntry *entryPtr = entry.get();
	connect(card, &QToolButton::clicked, this, [this, entryPtr]() { ActivateMedia(entryPtr); });

	QPixmap thumbnail;
	if (isImage) {
		QImageReader reader(path);
		reader.setAutoTransform(true);
		reader.setScaledSize(QSize(kThumbnailWidth, kThumbnailHeight));
		thumbnail = QPixmap::fromImage(reader.read());
	}
	if (thumbnail.isNull()) {
		thumbnail = PlaceholderForSource(entry->source);
	}
	SetCardThumbnail(entryPtr, thumbnail);

	if (!isImage) {
		entry->thumbnailView = App()->thumbnails()->createView(card, entry->source);
		connect(entry->thumbnailView, &ThumbnailView::updated, card,
			[this, entryPtr](const QPixmap &pixmap) { SetCardThumbnail(entryPtr, pixmap); });
		obs_source_inc_showing(entry->source);
		entry->thumbnailShowing = true;
		entry->thumbnailPrepareTimer = new QTimer(this);
		entry->thumbnailPrepareTimer->setSingleShot(true);
		connect(entry->thumbnailPrepareTimer, &QTimer::timeout, this, [entryPtr]() {
			obs_source_media_set_time(entryPtr->source, 0);
			obs_source_media_play_pause(entryPtr->source, true);
			if (entryPtr->thumbnailView) {
				entryPtr->thumbnailView->requestUpdate();
			}
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

	if (emptyState) {
		emptyState->hide();
	}
	const int index = static_cast<int>(entries.size());
	const int columns = 2;
	mediaGrid->addWidget(card, index / columns, index % columns);
	entries.emplace_back(std::move(entry));
	mediaCount->setText(tr("%1 archivos").arg(static_cast<qulonglong>(entries.size())));
}

QPixmap PresenterPanel::PlaceholderForSource(obs_source_t *source) const
{
	QPixmap result(kThumbnailWidth, kThumbnailHeight);
	result.fill(QColor("#111318"));
	const QIcon icon = main->GetSourceIcon(obs_source_get_id(source));
	const QPixmap sourceIcon = icon.pixmap(QSize(64, 64));
	QPainter painter(&result);
	painter.drawPixmap((result.width() - sourceIcon.width()) / 2, (result.height() - sourceIcon.height()) / 2,
			   sourceIcon);
	return result;
}

void PresenterPanel::SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap)
{
	if (!entry || !entry->button || pixmap.isNull()) {
		return;
	}
	entry->button->setIcon(QIcon(pixmap.scaled(kThumbnailWidth, kThumbnailHeight, Qt::KeepAspectRatio,
						   Qt::SmoothTransformation)));
}

void PresenterPanel::ActivateMedia(MediaEntry *entry)
{
	if (!entry || !entry->source || !stageScene) {
		return;
	}
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
	if (!activeItem) {
		return;
	}

	obs_sceneitem_set_bounds_type(activeItem, OBS_BOUNDS_SCALE_INNER);
	obs_sceneitem_set_bounds_alignment(activeItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_alignment(activeItem, OBS_ALIGN_CENTER);
	const struct vec2 size = {(float)obs_source_get_width(obs_scene_get_source(stageScene)),
				  (float)obs_source_get_height(obs_scene_get_source(stageScene))};
	const struct vec2 position = {size.x / 2.0f, size.y / 2.0f};
	obs_sceneitem_set_bounds(activeItem, &size);
	obs_sceneitem_set_pos(activeItem, &position);
	obs_source_media_restart(activeSource);

	for (const auto &candidate : entries) {
		if (!candidate->button) {
			continue;
		}
		candidate->button->setProperty("active", candidate.get() == entry);
		candidate->button->style()->unpolish(candidate->button);
		candidate->button->style()->polish(candidate->button);
	}
	currentMedia->setText(QFileInfo(entry->path).fileName());
}

void PresenterPanel::RefreshMonitorMenu()
{
	monitorMenu->clear();
	const auto screens = QGuiApplication::screens();
	const auto descriptions = OBSBasic::GetProjectorMenuMonitorsFormatted();
	for (int i = 0; i < screens.size(); ++i) {
		QAction *action = monitorMenu->addAction(descriptions.value(i, screens[i]->name()));
		action->setCheckable(true);
		action->setChecked(i == selectedMonitor);
		action->setProperty("monitor", i);
		connect(action, &QAction::triggered, this, &PresenterPanel::SelectMonitor);
	}
	if (screens.isEmpty()) {
		QAction *empty = monitorMenu->addAction(tr("No hay pantallas disponibles"));
		empty->setEnabled(false);
	}
}

void PresenterPanel::SelectMonitor()
{
	auto *action = qobject_cast<QAction *>(sender());
	if (!action || !stageScene) {
		return;
	}
	const int monitor = action->property("monitor").toInt();
	if (monitor < 0 || monitor >= QGuiApplication::screens().size()) {
		return;
	}
	if (stageProjector) {
		main->DeleteProjector(stageProjector);
		stageProjector = nullptr;
	}
	selectedMonitor = monitor;
	stageProjector = main->OpenPresenterProjector(obs_scene_get_source(stageScene), selectedMonitor);
	OBSProjector *projector = stageProjector;
	if (projector) {
		connect(projector, &QObject::destroyed, this, [this, projector]() {
			if (!stageProjector || stageProjector.data() == projector) {
				stageProjector = nullptr;
				selectedMonitor = -1;
				UpdateStageStatus();
			}
		});
	}
	UpdateStageStatus();
}

void PresenterPanel::UpdateStageStatus()
{
	QString text = tr("Escenario: sin pantalla");
	QString menuText = tr("Sin pantalla seleccionada");
	const auto screens = QGuiApplication::screens();
	if (selectedMonitor >= 0 && selectedMonitor < screens.size()) {
		const QString name = screens[selectedMonitor]->name().simplified();
		text = tr("Escenario: %1").arg(name);
		menuText = name;
	}
	if (stageStatus) {
		stageStatus->setText(text);
	}
	if (menuStageStatus) {
		menuStageStatus->setText(menuText);
	}
}

void PresenterPanel::RenderPreview(void *data, uint32_t cx, uint32_t cy)
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel || !panel->stageScene) {
		return;
	}
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
