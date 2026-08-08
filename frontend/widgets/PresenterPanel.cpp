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
#include "PresentationImporter.hpp"

#include <OBSApp.hpp>
#include <components/Multiview.hpp>
#include <components/AbsoluteSlider.hpp>
#include <utility/ThumbnailManager.hpp>
#include <utility/ThumbnailView.hpp>
#include <utility/display-helpers.hpp>

#include <obs-audio-controls.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFrame>
#include <QFormLayout>
#include <QGuiApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QProgressBar>
#include <QProgressDialog>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QScrollArea>
#include <QSaveFile>
#include <QSettings>
#include <QSignalBlocker>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>

#include "moc_PresenterPanel.cpp"

#ifndef OPBS_VERSION
#define OPBS_VERSION "0.1.0"
#endif

namespace {
constexpr int kThumbnailWidth = 256;
constexpr int kThumbnailHeight = 144;
constexpr int kBibleResultLimit = 250;
constexpr auto kBibleFolderId = "presentation-bible";
constexpr auto kPresentationsFolderId = "presentation-slides";
const QStringList imageExtensions = {"bmp", "gif", "jpeg", "jpg", "png", "tga", "webp"};
const QStringList audioExtensions = {"mp3", "aac", "ogg", "wav", "flac", "m4a", "wma"};
const QStringList videoExtensions = {"mp4", "m4v", "mov", "mkv", "avi", "webm", "wmv", "mpeg", "mpg"};

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
	QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override
	{
		QMimeData *data = QListWidget::mimeData(items);
		QStringList paths;
		for (QListWidgetItem *item : items)
			paths.push_back(item->data(Qt::UserRole).toString());
		data->setData("application/x-presenter-media-paths", paths.join('\n').toUtf8());
		return data;
	}

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

class PresenterFolderList final : public QListWidget {
public:
	std::function<void(const QStringList &, const QString &)> filesDropped;
	std::function<void(const QStringList &, const QString &)> mediaMoved;
	std::function<void()> orderChanged;

	explicit PresenterFolderList(QWidget *parent) : QListWidget(parent)
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
		if (event->mimeData()->hasUrls() || event->mimeData()->hasFormat("application/x-presenter-media-paths")) {
			event->acceptProposedAction();
			return;
		}
		QListWidget::dragEnterEvent(event);
	}

	void dragMoveEvent(QDragMoveEvent *event) override
	{
		if (event->mimeData()->hasUrls() || event->mimeData()->hasFormat("application/x-presenter-media-paths")) {
			event->acceptProposedAction();
			return;
		}
		QListWidget::dragMoveEvent(event);
	}

	void dropEvent(QDropEvent *event) override
	{
		QListWidgetItem *target = itemAt(event->position().toPoint());
		if (!target)
			target = currentItem();
		const QString folderId = target ? target->data(Qt::UserRole).toString() : QString();
		if (!folderId.isEmpty() && event->mimeData()->hasUrls()) {
			QStringList paths;
			for (const QUrl &url : event->mimeData()->urls()) {
				if (url.isLocalFile())
					paths.push_back(url.toLocalFile());
			}
			if (!paths.isEmpty() && filesDropped)
				filesDropped(paths, folderId);
			event->acceptProposedAction();
			return;
		}
		if (!folderId.isEmpty() && event->mimeData()->hasFormat("application/x-presenter-media-paths")) {
			const QStringList paths = QString::fromUtf8(event->mimeData()->data("application/x-presenter-media-paths"))
							  .split('\n', Qt::SkipEmptyParts);
			if (mediaMoved)
				mediaMoved(paths, folderId);
			event->acceptProposedAction();
			return;
		}
		QListWidget::dropEvent(event);
		if (orderChanged)
			orderChanged();
	}
};

class BibleLayoutPreview final : public QWidget {
	QString fontFamily = QStringLiteral("Arial");
	QString textAlignment = QStringLiteral("center");
	QString referencePosition = QStringLiteral("bottom-center");
	QString backgroundPath;
	int fontSize = 96;

public:
	explicit BibleLayoutPreview(QWidget *parent = nullptr) : QWidget(parent)
	{
		setMinimumSize(480, 270);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}

	bool hasHeightForWidth() const override { return true; }
	int heightForWidth(int width) const override { return std::max(270, width * 9 / 16); }
	QSize sizeHint() const override { return QSize(640, 360); }

	void SetLayout(const QString &family, int size, const QString &alignment, const QString &reference,
		       const QString &background)
	{
		fontFamily = family;
		fontSize = size;
		textAlignment = alignment;
		referencePosition = reference;
		backgroundPath = background;
		update();
	}

protected:
	void paintEvent(QPaintEvent *) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.fillRect(rect(), QColor(Qt::black));
		if (!backgroundPath.isEmpty()) {
			const QPixmap background(backgroundPath);
			if (!background.isNull()) {
				const QPixmap scaled =
					background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
				const QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
				painter.drawPixmap(topLeft, scaled);
			} else {
				painter.fillRect(rect(), QColor("#172033"));
				painter.setPen(QColor("#aeb8d0"));
				painter.drawText(rect().adjusted(12, 10, -12, -10),
						 Qt::AlignBottom | Qt::AlignRight,
						 QObject::tr("Fondo de video: %1")
							 .arg(QFileInfo(backgroundPath).fileName()));
			}
		}
		const QRect content = rect().adjusted(width() / 16, height() / 14, -width() / 16, -height() / 14);
		const bool referenceAtTop = referencePosition.startsWith(QStringLiteral("top"));
		const QRect referenceRect(content.left(), referenceAtTop ? content.top() : content.bottom() - height() / 10,
					  content.width(), height() / 10);
		const QRect verseRect(content.left(), referenceAtTop ? referenceRect.bottom() + height() / 30 : content.top(),
				      content.width(), content.height() - referenceRect.height() - height() / 24);
		Qt::Alignment verseAlignment = Qt::AlignVCenter;
		if (textAlignment == QStringLiteral("left"))
			verseAlignment |= Qt::AlignLeft;
		else if (textAlignment == QStringLiteral("right"))
			verseAlignment |= Qt::AlignRight;
		else
			verseAlignment |= Qt::AlignHCenter;
		Qt::Alignment referenceAlignment = Qt::AlignVCenter;
		if (referencePosition.endsWith(QStringLiteral("left")))
			referenceAlignment |= Qt::AlignLeft;
		else if (referencePosition.endsWith(QStringLiteral("right")))
			referenceAlignment |= Qt::AlignRight;
		else
			referenceAlignment |= Qt::AlignHCenter;
		painter.setPen(Qt::white);
		QFont verseFont(fontFamily);
		verseFont.setPixelSize(std::max(12, fontSize * height() / 1080));
		painter.setFont(verseFont);
		painter.drawText(verseRect, verseAlignment | Qt::TextWordWrap,
				 QObject::tr("El Señor es mi pastor; nada me faltará."));
		QFont referenceFont(fontFamily);
		referenceFont.setPixelSize(std::max(10, int(fontSize * 0.55) * height() / 1080));
		painter.setFont(referenceFont);
		painter.drawText(referenceRect, referenceAlignment, QObject::tr("Salmos 23:1"));
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

QString NormalizeBibleText(const QString &value)
{
	const QString decomposed = value.normalized(QString::NormalizationForm_D).toCaseFolded();
	QString normalized;
	normalized.reserve(decomposed.size());
	for (const QChar character : decomposed) {
		const QChar::Category category = character.category();
		if (category != QChar::Mark_NonSpacing && category != QChar::Mark_SpacingCombining &&
		    category != QChar::Mark_Enclosing)
			normalized.append(character);
	}
	return normalized.simplified();
}

QString BibleDisplayName(const QString &path)
{
	QString name = QFileInfo(path).completeBaseName();
	name.replace('_', ' ');
	name.replace('-', ' ');
	if (name.startsWith("biblia ", Qt::CaseInsensitive))
		name.remove(0, 7);
	if (NormalizeBibleText(name) == QStringLiteral("reina valera 1960"))
		return QStringLiteral("Reina Valera 1960");
	const QStringList words = name.simplified().split(' ', Qt::SkipEmptyParts);
	QStringList titleWords;
	for (QString word : words) {
		if (!word.isEmpty())
			word[0] = word[0].toUpper();
		titleWords.push_back(word);
	}
	return titleWords.join(' ');
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
		#presenterFolderPanel { background: #15181f; border: 1px solid #2a2e38; border-radius: 8px; }
		QListWidget#presenterFolderList, QListWidget#presenterPresentationList {
			background: transparent; border: 0; outline: 0; }
		QListWidget#presenterFolderList::item, QListWidget#presenterPresentationList::item {
			padding: 8px; margin: 2px; border-radius: 6px; }
		QListWidget#presenterFolderList::item:selected, QListWidget#presenterPresentationList::item:selected {
			background: #343052; color: white; }
		QLineEdit#presenterSearch { background: #20242d; color: #eef0f6; border: 1px solid #303643;
			border-radius: 7px; padding: 8px 11px; }
		QLineEdit#presenterSearch:focus { border-color: #7b6cff; }
		#presenterBibleControls { background: #15181f; border: 1px solid #2a2e38; border-radius: 8px; }
		QComboBox#presenterBibleSelector { background: #20242d; color: #eef0f6; border: 1px solid #303643;
			border-radius: 7px; padding: 8px 11px; min-width: 190px; }
		QComboBox#presenterBibleSelector::drop-down { border: 0; width: 28px; }
		QListWidget#presenterBibleList { background: transparent; border: 0; outline: 0; }
		QListWidget#presenterBibleList::item { background: #20242d; color: #eef0f6; border: 1px solid #303643;
			border-radius: 9px; padding: 12px; }
		QListWidget#presenterBibleList::item:hover { border-color: #6d5dfc; background: #252a35; }
		QListWidget#presenterBibleList::item:selected { border: 2px solid #7b6cff; background: #29263d; }
		QProgressBar#presenterMeter { background: #252a33; border: 0; border-radius: 3px; max-height: 7px; }
		QProgressBar#presenterMeter::chunk { background: #42d17c; border-radius: 3px; }
		QSlider::groove:horizontal { height: 6px; background: #303643; border-radius: 3px; }
		QSlider::handle:horizontal { background: #7b6cff; width: 15px; margin: -5px 0; border-radius: 7px; }
		QCheckBox#presenterStageToggle { spacing: 7px; color: #d7dbea; }
		QToolButton#presenterTransport { background: #252a35; color: #f4f6fb; border: 1px solid #373e4c;
			border-radius: 7px; min-width: 38px; min-height: 30px; font-size: 16px; font-weight: 700; }
		QToolButton#presenterTransport:hover { background: #313747; border-color: #7b6cff; }
		QToolButton#presenterTransport:checked { background: #6d5dfc; border-color: #8d82ff; color: white; }
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
	auto *title = new QLabel(tr("OPBS"), header);
	title->setObjectName("presenterTitle");
	auto *subtitle =
		new QLabel(tr("Presentador multimedia integrado · Versión %1").arg(QString::fromLatin1(OPBS_VERSION)), header);
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
	timelineSlider = new AbsoluteSlider(Qt::Horizontal, previewFrame);
	timelineSlider->setRange(0, 1000);
	timelineSlider->setEnabled(false);
	timeLabel = new QLabel("0:00 / 0:00", previewFrame);
	timeLabel->setObjectName("presenterTime");
	timelineRow->addWidget(timelineSlider, 1);
	timelineRow->addWidget(timeLabel);
	previewLayout->addLayout(timelineRow);
	connect(timelineSlider, &QSlider::sliderPressed, this, [this]() {
		timelineDragging = true;
		pendingSeekValue = timelineSlider->value();
	});
	connect(timelineSlider, &QSlider::sliderMoved, this, [this](int value) {
		if (!activeSource)
			return;
		pendingSeekValue = value;
		seekTimer->start(75);
		seekGuardUntil = QDateTime::currentMSecsSinceEpoch() + 1500;
		const int64_t observedDuration = obs_source_media_get_duration(activeSource);
		if (observedDuration > 0)
			cachedDuration = observedDuration;
		const int64_t duration = cachedDuration;
		if (duration > 0)
			timeLabel->setText(QString("%1 / %2").arg(FormatTime(duration * value / 1000), FormatTime(duration)));
	});
	connect(timelineSlider, &QSlider::sliderReleased, this, [this]() {
		seekTimer->stop();
		pendingSeekValue = timelineSlider->value();
		const int value = pendingSeekValue;
		const int64_t duration = cachedDuration;
		OBSSource source = activeSource;
		timelineDragging = false;
		seekGuardUntil = QDateTime::currentMSecsSinceEpoch() + 1500;
		if (source && duration > 0)
			obs_source_media_set_time_immediate(source, duration * value / 1000);
		/* AbsoluteSlider updates its value on mouse press.  Retry shortly
		 * after release so a click remains seekable even when the release
		 * notification races with media cache startup. */
		seekTimer->start(120);
	});

	auto *transportRow = new QHBoxLayout();
	transportRow->addStretch();
	auto makeTransportButton = [previewFrame, transportRow](QStyle::StandardPixmap icon, const QString &tip) {
		auto *button = new QToolButton(previewFrame);
		button->setObjectName("presenterTransport");
		button->setIcon(previewFrame->style()->standardIcon(icon));
		button->setIconSize(QSize(20, 20));
		button->setToolTip(tip);
		button->setAccessibleName(tip);
		transportRow->addWidget(button);
		return button;
	};
	auto *previousButton = makeTransportButton(QStyle::SP_MediaSkipBackward, tr("Anterior (tecla multimedia anterior)"));
	playPauseButton = makeTransportButton(QStyle::SP_MediaPlay, tr("Reproducir / pausar (tecla multimedia)"));
	auto *stopButton = makeTransportButton(QStyle::SP_MediaStop, tr("Detener (tecla multimedia detener)"));
	auto *nextButton = makeTransportButton(QStyle::SP_MediaSkipForward, tr("Siguiente (tecla multimedia siguiente)"));
	loopButton = makeTransportButton(QStyle::SP_BrowserReload, tr("Repetir continuamente el archivo actual"));
	loopButton->setCheckable(true);
	loopButton->setEnabled(false);
	transportRow->addStretch();
	previewLayout->addLayout(transportRow);
	connect(previousButton, &QToolButton::clicked, this, &PresenterPanel::PreviousMedia);
	connect(playPauseButton, &QToolButton::clicked, this, &PresenterPanel::TogglePlayPause);
	connect(stopButton, &QToolButton::clicked, this, &PresenterPanel::StopMedia);
	connect(nextButton, &QToolButton::clicked, this, &PresenterPanel::NextMedia);
	connect(loopButton, &QToolButton::toggled, this, [this](bool enabled) {
		loopCurrent = enabled;
		ApplyLoopSetting();
		if (!restoring)
			SaveSettings();
	});

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
	auto *libraryBody = new QHBoxLayout();
	libraryBody->setSpacing(12);
	auto *folderPanel = new QFrame(library);
	folderPanel->setObjectName("presenterFolderPanel");
	folderPanel->setFixedWidth(165);
	auto *folderLayout = new QVBoxLayout(folderPanel);
	folderLayout->setContentsMargins(8, 8, 8, 8);
	folderLayout->addWidget(new QLabel(tr("Multimedia"), folderPanel));
	auto *foldersWidget = new PresenterFolderList(folderPanel);
	folderList = foldersWidget;
	foldersWidget->setObjectName("presenterFolderList");
	foldersWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	foldersWidget->setMinimumHeight(220);
	folderLayout->addWidget(foldersWidget, 65);
	auto *folderButtons = new QHBoxLayout();
	auto *addFolder = new QToolButton(folderPanel);
	addFolder->setText("+");
	addFolder->setToolTip(tr("Crear carpeta"));
	auto *renameFolder = new QToolButton(folderPanel);
	renameFolder->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
	renameFolder->setToolTip(tr("Renombrar carpeta"));
	folderButtons->addWidget(addFolder);
	folderButtons->addWidget(renameFolder);
	folderButtons->addStretch();
	folderLayout->addLayout(folderButtons);
	folderLayout->addWidget(new QLabel(tr("Presentación"), folderPanel));
	auto *presentationsWidget = new PresenterFolderList(folderPanel);
	presentationFolderList = presentationsWidget;
	presentationsWidget->setObjectName("presenterPresentationList");
	presentationsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	presentationsWidget->setDragEnabled(false);
	presentationsWidget->setDragDropMode(QAbstractItemView::DropOnly);
	presentationsWidget->setFixedHeight(92);
	auto addPresentationFolder = [this, presentationsWidget](const QString &id, const QString &name) {
		auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), name, presentationsWidget);
		item->setData(Qt::UserRole, id);
		item->setFlags((item->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsEditable) | Qt::ItemIsDropEnabled);
	};
	addPresentationFolder(QString::fromLatin1(kBibleFolderId), tr("Biblia"));
	addPresentationFolder(QString::fromLatin1(kPresentationsFolderId), tr("Presentaciones"));
	folderLayout->addWidget(presentationsWidget);
	folderLayout->addStretch(35);
	libraryBody->addWidget(folderPanel);
	auto *mediaArea = new QVBoxLayout();
	auto *bibleToolbar = new QFrame(library);
	bibleControls = bibleToolbar;
	bibleToolbar->setObjectName("presenterBibleControls");
	auto *bibleToolbarLayout = new QHBoxLayout(bibleToolbar);
	bibleToolbarLayout->setContentsMargins(10, 8, 10, 8);
	bibleSearchEdit = new QLineEdit(bibleToolbar);
	bibleSearchEdit->setObjectName("presenterSearch");
	bibleSearchEdit->setPlaceholderText(tr("Buscar texto o referencia bíblica…"));
	bibleSearchEdit->setClearButtonEnabled(true);
	bibleSelector = new QComboBox(bibleToolbar);
	bibleSelector->setObjectName("presenterBibleSelector");
	bibleSelector->setToolTip(tr("Biblia seleccionada"));
	bibleToolbarLayout->addWidget(bibleSearchEdit, 1);
	bibleToolbarLayout->addWidget(bibleSelector);
	bibleToolbar->hide();
	mediaArea->addWidget(bibleToolbar);
	emptyState = new QLabel(tr("Arrastra aquí imágenes, videos o audio para comenzar"), library);
	emptyState->setObjectName("presenterEmpty");
	emptyState->setAlignment(Qt::AlignCenter);
	emptyState->setMinimumHeight(220);
	mediaArea->addWidget(emptyState, 1);
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
	list->setContextMenuPolicy(Qt::CustomContextMenu);
	list->hide();
	list->filesDropped = [this](const QStringList &paths) { ImportPaths(paths, SelectedFolderId()); };
	list->orderChanged = [this]() { ReorderEntriesFromList(); };
	connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
		for (const auto &entry : entries) {
			if (entry->item == item) {
				ActivateMedia(entry.get());
				break;
			}
		}
	});
	connect(list, &QWidget::customContextMenuRequested, this, [this, list](const QPoint &position) {
		if (!folderList || !folderList->currentItem())
			return;
		QListWidgetItem *item = list->itemAt(position);
		if (!item)
			return;
		auto found = std::find_if(entries.begin(), entries.end(),
					 [item](const auto &entry) { return entry->item == item; });
		if (found == entries.end())
			return;
		QMenu menu(list);
		QAction *removeAction = menu.addAction(style()->standardIcon(QStyle::SP_TrashIcon), tr("Eliminar"));
		if (menu.exec(list->viewport()->mapToGlobal(position)) == removeAction)
			RemoveMediaEntry(found->get());
	});
	connect(list->verticalScrollBar(), &QScrollBar::valueChanged, this,
		[this]() { QTimer::singleShot(0, this, &PresenterPanel::LoadVisibleThumbnails); });
	mediaArea->addWidget(list, 1);
	bibleResultsList = new QListWidget(library);
	bibleResultsList->setObjectName("presenterBibleList");
	bibleResultsList->setViewMode(QListView::IconMode);
	bibleResultsList->setResizeMode(QListView::Adjust);
	bibleResultsList->setMovement(QListView::Static);
	bibleResultsList->setWrapping(true);
	bibleResultsList->setWordWrap(true);
	bibleResultsList->setUniformItemSizes(true);
	bibleResultsList->setGridSize(QSize(310, 165));
	bibleResultsList->setSpacing(8);
	bibleResultsList->setSelectionMode(QAbstractItemView::SingleSelection);
	bibleResultsList->hide();
	connect(bibleResultsList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
		if (item)
			ProjectBibleVerse(item->data(Qt::UserRole + 1).toString(), item->data(Qt::UserRole).toString());
	});
	mediaArea->addWidget(bibleResultsList, 1);
	searchEdit = new QLineEdit(library);
	searchEdit->setObjectName("presenterSearch");
	searchEdit->setPlaceholderText(tr("Buscar en esta carpeta…"));
	searchEdit->setClearButtonEnabled(true);
	mediaArea->addWidget(searchEdit);
	libraryBody->addLayout(mediaArea, 1);
	libraryLayout->addLayout(libraryBody, 1);

	connect(addFolder, &QToolButton::clicked, this, &PresenterPanel::CreateFolder);
	connect(renameFolder, &QToolButton::clicked, this, &PresenterPanel::RenameFolder);
	connect(foldersWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
		if (!current)
			return;
		if (presentationFolderList)
			presentationFolderList->setCurrentRow(-1);
		currentFolderId = current->data(Qt::UserRole).toString();
		ApplyLibraryFilter();
		if (!restoring)
			SaveSettings();
	});
	connect(presentationsWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
		if (!current)
			return;
		if (folderList)
			folderList->setCurrentRow(-1);
		currentFolderId = current->data(Qt::UserRole).toString();
		ApplyLibraryFilter();
		if (!restoring)
			SaveSettings();
	});
	connect(foldersWidget, &QListWidget::itemChanged, this, [this]() {
		if (!restoring)
			SaveSettings();
	});
	foldersWidget->filesDropped = [this](const QStringList &paths, const QString &folderId) { ImportPaths(paths, folderId); };
	foldersWidget->mediaMoved = [this](const QStringList &paths, const QString &folderId) { MoveMediaToFolder(paths, folderId); };
	foldersWidget->orderChanged = [this]() { ReorderFoldersFromList(); };
	presentationsWidget->filesDropped =
		[this](const QStringList &paths, const QString &folderId) { ImportPaths(paths, folderId); };
	presentationsWidget->mediaMoved =
		[this](const QStringList &paths, const QString &folderId) { MoveMediaToFolder(paths, folderId); };
	connect(searchEdit, &QLineEdit::textChanged, this, [this]() { ApplyLibraryFilter(); });
	connect(bibleSearchEdit, &QLineEdit::textChanged, this, [this](const QString &query) {
		QTimer::singleShot(140, this, [this, query]() {
			if (bibleSearchEdit && bibleSearchEdit->text() == query)
				ApplyBibleFilter();
		});
	});
	connect(bibleSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		LoadBibleTranslation(index);
		ApplyBibleFilter();
		if (!restoring)
			SaveSettings();
	});

	splitter->addWidget(previewFrame);
	splitter->addWidget(library);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({600, 600});
	root->addWidget(splitter, 1);

	timelineTimer = new QTimer(this);
	timelineTimer->setInterval(250);
	connect(timelineTimer, &QTimer::timeout, this, &PresenterPanel::RefreshTimeline);
	seekTimer = new QTimer(this);
	seekTimer->setSingleShot(true);
	connect(seekTimer, &QTimer::timeout, this, &PresenterPanel::SeekToPendingPosition);
}

void PresenterPanel::BuildTopMenu()
{
	auto *fileMenu = main->menuBar()->addMenu(tr("Archivo"));
	fileMenuAction = fileMenu->menuAction();
	auto *importMenu = fileMenu->addMenu(tr("Importar"));
	QAction *importPowerPoint = importMenu->addAction(tr("PowerPoint"));
	QAction *importPdf = importMenu->addAction(tr("PDF"));
	auto *editMenu = main->menuBar()->addMenu(tr("Editar"));
	editMenuAction = editMenu->menuAction();
	fitToScreenAction = editMenu->addAction(tr("Activar ajustar a tamaño de pantalla"));
	fitToScreenAction->setCheckable(true);
	screensAction = main->menuBar()->addAction(tr("Pantallas"));
	soundAction = main->menuBar()->addAction(tr("Sonido"));
	bibleAction = main->menuBar()->addAction(tr("Biblia"));
	auto *helpMenu = main->menuBar()->addMenu(tr("Ayuda"));
	helpMenuAction = helpMenu->menuAction();
	opbsUpdateAction = helpMenu->addAction(tr("Buscar actualizaciones de OPBS"));
	helpMenu->addSeparator();
	helpMenu->addAction(tr("Acerca de OPBS"), this, [this]() {
		QMessageBox::about(main, tr("Acerca de OPBS"),
				   tr("OPBS %1\nPresentador integrado basado en OBS Studio.")
					   .arg(QString::fromLatin1(OPBS_VERSION)));
	});
	connect(fitToScreenAction, &QAction::toggled, this, [this](bool enabled) {
		fitContentToScreen = enabled;
		ApplyActiveItemBounds();
		if (!restoring)
			SaveSettings();
	});
	connect(screensAction, &QAction::triggered, this, &PresenterPanel::ShowScreensDialog);
	connect(soundAction, &QAction::triggered, this, &PresenterPanel::ShowSoundDialog);
	connect(bibleAction, &QAction::triggered, this, &PresenterPanel::ShowBibleDialog);
	connect(opbsUpdateAction, &QAction::triggered, this, &PresenterPanel::LaunchOpbsUpdater);
	connect(importPowerPoint, &QAction::triggered, this, [this]() { ImportPresentation(false); });
	connect(importPdf, &QAction::triggered, this, [this]() { ImportPresentation(true); });
	for (QAction *action : main->menuBar()->actions())
		action->setVisible(action == fileMenuAction || action == editMenuAction || action == screensAction ||
				   action == soundAction || action == bibleAction || action == helpMenuAction);

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

void PresenterPanel::LaunchOpbsUpdater(bool silent)
{
#ifdef _WIN32
	const QString applicationDirectory = QCoreApplication::applicationDirPath();
	const QString updaterPath = QDir(applicationDirectory).filePath(QStringLiteral("OPBS-Updater.ps1"));
	if (!QFileInfo::exists(updaterPath)) {
		QMessageBox::critical(main, tr("Actualizaciones de OPBS"),
				      tr("No se encontró el componente de actualización de OPBS."));
		return;
	}

	QStringList arguments = {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"),
				 QStringLiteral("Bypass"), QStringLiteral("-File"), updaterPath,
				 QStringLiteral("-CurrentProcessId"),
				 QString::number(QCoreApplication::applicationPid())};
	if (silent)
		arguments.append(QStringLiteral("-Silent"));
	if (!QProcess::startDetached(QStringLiteral("powershell.exe"), arguments, applicationDirectory)) {
		QMessageBox::critical(main, tr("Actualizaciones de OPBS"),
				      tr("No fue posible iniciar el comprobador de actualizaciones."));
	}
#else
	QMessageBox::information(main, tr("Actualizaciones de OPBS"),
				 tr("El actualizador de OPBS está disponible actualmente para Windows."));
#endif
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
	main->setWindowTitle(tr("OPBS %1 — Presentador integrado").arg(QString::fromLatin1(OPBS_VERSION)));
	QTimer::singleShot(8000, this, [this]() { LaunchOpbsUpdater(true); });
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
	// El presentador usa únicamente el monitor de su fuente activa. Las entradas
	// globales de OBS (escritorio y micrófono) pueden bloquear ese mismo dispositivo.
	for (uint32_t channel = 1; channel < 6; ++channel)
		obs_set_output_source(channel, nullptr);
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
	ClearActiveMedia();
	ClearBiblePresentation();
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
	ImportPaths(QFileDialog::getOpenFileNames(this, tr("Importar contenido multimedia"), QString(), filter),
		    SelectedFolderId());
}

void PresenterPanel::ImportPresentation(bool pdf)
{
	const QString filter = pdf ? tr("Documento PDF (*.pdf)")
				   : tr("Presentación de PowerPoint (*.ppt *.pptx)");
	const QString path = QFileDialog::getOpenFileName(
		main, pdf ? tr("Importar PDF") : tr("Importar PowerPoint"), QString(), filter);
	if (path.isEmpty())
		return;

	const QString basePath = PresentationsDirectoryPath();
	QDir().mkpath(basePath);
	const QString temporaryName = QStringLiteral("import-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString temporaryPath = QDir(basePath).filePath(temporaryName);
	QDir().mkpath(temporaryPath);

	QProgressDialog progress(pdf ? tr("Convirtiendo páginas del PDF…")
				     : tr("Convirtiendo diapositivas de PowerPoint…"),
				 QString(), 0, 0, main);
	progress.setWindowTitle(tr("Importar presentación"));
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	progress.show();
	QApplication::processEvents();

	const PresentationImportResult result =
		pdf ? PresentationImporter::ImportPdf(path, temporaryPath)
		    : PresentationImporter::ImportPowerPoint(path, temporaryPath);
	progress.close();
	if (!result.success) {
		QDir(temporaryPath).removeRecursively();
		QMessageBox::critical(main, tr("No se pudo importar la presentación"),
				      result.error.isEmpty() ? tr("No se generaron diapositivas.") : result.error);
		return;
	}
	ReplacePresentationSlides(temporaryPath, result.slidePaths.size());
}

void PresenterPanel::ClearPresentationEntries()
{
	for (auto iterator = entries.begin(); iterator != entries.end();) {
		MediaEntry *entry = iterator->get();
		if (entry->folderId != QString::fromLatin1(kPresentationsFolderId)) {
			++iterator;
			continue;
		}
		if (activeEntry == entry)
			ClearActiveMedia();
		else
			ReleaseSource(entry);
		if (entry->thumbnailView)
			delete entry->thumbnailView.data();
		entry->thumbnailView = nullptr;
		delete entry->item;
		entry->item = nullptr;
		iterator = entries.erase(iterator);
	}
}

void PresenterPanel::ReplacePresentationSlides(const QString &temporaryDirectory, int slideCount)
{
	const QString basePath = PresentationsDirectoryPath();
	QDir base(basePath);
	const QString currentName = QStringLiteral("current");
	const QString backupName =
		QStringLiteral("previous-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString temporaryName = QFileInfo(temporaryDirectory).fileName();
	const bool hadCurrent = QDir(base.filePath(currentName)).exists();
	const auto restoreCurrentEntries = [this, &base, &currentName]() {
		const QString currentPath = base.filePath(currentName);
		for (int index = 1;; ++index) {
			const QString path = QDir(currentPath).filePath(QString::number(index) + ".png");
			if (!QFileInfo::exists(path))
				break;
			AddMediaFile(path, QString::fromLatin1(kPresentationsFolderId), false);
		}
		ApplyLibraryFilter();
		SaveSettings();
	};

	ClearPresentationEntries();
	if (hadCurrent && !base.rename(currentName, backupName)) {
		QDir(temporaryDirectory).removeRecursively();
		restoreCurrentEntries();
		QMessageBox::critical(main, tr("Importar presentación"),
				      tr("No se pudo reemplazar la presentación anterior."));
		return;
	}
	if (!base.rename(temporaryName, currentName)) {
		if (hadCurrent)
			base.rename(backupName, currentName);
		restoreCurrentEntries();
		QMessageBox::critical(main, tr("Importar presentación"),
				      tr("No se pudo activar la nueva presentación."));
		return;
	}
	if (hadCurrent)
		QDir(base.filePath(backupName)).removeRecursively();

	const QString currentPath = base.filePath(currentName);
	for (int index = 1; index <= slideCount; ++index)
		AddMediaFile(QDir(currentPath).filePath(QString::number(index) + ".png"),
			     QString::fromLatin1(kPresentationsFolderId), false);
	if (presentationFolderList) {
		for (int row = 0; row < presentationFolderList->count(); ++row) {
			if (presentationFolderList->item(row)->data(Qt::UserRole).toString() ==
			    QString::fromLatin1(kPresentationsFolderId)) {
				presentationFolderList->setCurrentRow(row);
				break;
			}
		}
	}
	ApplyLibraryFilter();
	SaveSettings();
	QMessageBox::information(main, tr("Presentación importada"),
				 tr("Se importaron %1 diapositivas.").arg(slideCount));
}

void PresenterPanel::ImportPaths(const QStringList &paths, const QString &folderId)
{
	for (const QString &path : paths)
		AddMediaFile(path, folderId, false);
	ApplyLibraryFilter();
	SaveSettings();
}

void PresenterPanel::AddMediaFile(const QString &path, const QString &folderId, bool save)
{
	const QFileInfo info(path);
	if (!info.exists() || !info.isFile())
		return;
	const QString destinationFolder = folderId.isEmpty() ? SelectedFolderId() : folderId;
	for (const auto &existing : entries) {
		if (QFileInfo(existing->path).absoluteFilePath() == info.absoluteFilePath()) {
			existing->folderId = destinationFolder;
			ApplyLibraryFilter();
			if (save)
				SaveSettings();
			return;
		}
	}
	const bool isImage = imageExtensions.contains(info.suffix().toLower());
	const char *sourceType = isImage ? "image_source" : "ffmpeg_source";
	auto entry = std::make_unique<MediaEntry>();
	entry->path = info.absoluteFilePath();
	entry->folderId = destinationFolder.isEmpty() ? QStringLiteral("general") : destinationFolder;
	entry->isImage = isImage;
	const QString cardName = entry->folderId == QString::fromLatin1(kPresentationsFolderId)
					 ? info.completeBaseName()
					 : info.fileName();
	entry->item = new QListWidgetItem(cardName, mediaList);
	entry->item->setData(Qt::UserRole, entry->path);
	entry->item->setToolTip(entry->path);
	entry->item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	MediaEntry *entryPtr = entry.get();
	SetCardThumbnail(entryPtr, PlaceholderForType(sourceType));
	entries.emplace_back(std::move(entry));
	ApplyLibraryFilter();
	if (save)
		SaveSettings();
}

QPixmap PresenterPanel::PlaceholderForType(const char *sourceType) const
{
	QPixmap result(kThumbnailWidth, kThumbnailHeight);
	result.fill(QColor("#111318"));
	const QPixmap icon = main->GetSourceIcon(sourceType).pixmap(QSize(64, 64));
	QPainter painter(&result);
	painter.drawPixmap((result.width() - icon.width()) / 2, (result.height() - icon.height()) / 2, icon);
	return result;
}

void PresenterPanel::SetCardThumbnail(MediaEntry *entry, const QPixmap &pixmap)
{
	if (entry && entry->item && !pixmap.isNull())
		entry->item->setIcon(QIcon(pixmap.scaled(kThumbnailWidth, kThumbnailHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void PresenterPanel::LoadThumbnail(MediaEntry *entry)
{
	if (!entry || entry->thumbnailLoaded || !entry->isImage)
		return;
	QImageReader reader(entry->path);
	reader.setAutoTransform(true);
	reader.setScaledSize(QSize(kThumbnailWidth, kThumbnailHeight));
	const QPixmap thumbnail = QPixmap::fromImage(reader.read());
	if (!thumbnail.isNull()) {
		SetCardThumbnail(entry, thumbnail);
		entry->thumbnailLoaded = true;
	}
}

void PresenterPanel::LoadVisibleThumbnails()
{
	if (!mediaList || !mediaList->isVisible())
		return;
	const QRect visibleArea = mediaList->viewport()->rect().adjusted(0, -kThumbnailHeight, 0, kThumbnailHeight);
	int preloadCount = 0;
	for (const auto &entry : entries) {
		if (!entry->item || entry->item->isHidden() || entry->thumbnailLoaded || !entry->isImage)
			continue;
		const QRect itemRect = mediaList->visualItemRect(entry->item);
		if (visibleArea.intersects(itemRect) || preloadCount < 24) {
			LoadThumbnail(entry.get());
			++preloadCount;
		}
	}
}

bool PresenterPanel::EnsureSource(MediaEntry *entry)
{
	if (!entry)
		return false;
	if (entry->source)
		return true;
	OBSDataAutoRelease settings = obs_data_create();
	const char *sourceType = entry->isImage ? "image_source" : "ffmpeg_source";
	if (entry->isImage) {
		obs_data_set_string(settings, "file", entry->path.toUtf8().constData());
	} else {
		const QString suffix = QFileInfo(entry->path).suffix().toLower();
		const bool audioOnly = audioExtensions.contains(suffix);
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_string(settings, "local_file", entry->path.toUtf8().constData());
		obs_data_set_bool(settings, "restart_on_activate", false);
		obs_data_set_bool(settings, "close_when_inactive", false);
		obs_data_set_bool(settings, "clear_on_media_end", true);
		obs_data_set_bool(settings, "looping", loopCurrent);
		/* Full decode uses OBS' frame cache, whose clock is designed around
		 * preloaded visual media.  Stream music normally so its timeline is
		 * driven by real audio playback time. */
		obs_data_set_bool(settings, "full_decode", false);
		obs_data_set_bool(settings, "audio_only", audioOnly);
	}
	sourceType = obs_get_latest_input_type_id(sourceType);
	if (!sourceType)
		return false;
	OBSSourceAutoRelease source =
		obs_source_create_private(sourceType, QFileInfo(entry->path).fileName().toUtf8().constData(), settings);
	entry->source = source.Get();
	return entry->source != nullptr;
}

void PresenterPanel::ReleaseSource(MediaEntry *entry)
{
	if (!entry || entry == activeEntry)
		return;
	if (entry->thumbnailView)
		delete entry->thumbnailView.data();
	entry->thumbnailView = nullptr;
	entry->source = nullptr;
}

void PresenterPanel::ClearActiveMedia(MediaEntry *keepEntry)
{
	MediaEntry *previousEntry = activeEntry;
	if (audioMeter)
		obs_volmeter_detach_source(audioMeter);
	DetachAudioFilters();
	if (activeSource) {
		signal_handler_disconnect(obs_source_get_signal_handler(activeSource), "media_started", MediaStarted, this);
		obs_source_media_stop(activeSource);
		obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_NONE);
	}
	if (activeItem) {
		obs_sceneitem_remove(activeItem);
		activeItem = nullptr;
	}
	activeSource = nullptr;
	activeEntry = nullptr;
	if (previousEntry && previousEntry != keepEntry)
		ReleaseSource(previousEntry);
	cachedDuration = 0;
	pendingSeekValue = -1;
	if (loopButton)
		loopButton->setEnabled(false);
	if (timelineSlider)
		timelineSlider->setValue(0);
	if (timeLabel)
		timeLabel->setText(QStringLiteral("0:00 / 0:00"));
	if (meterLeft)
		meterLeft->setValue(0);
	if (meterRight)
		meterRight->setValue(0);
}

void PresenterPanel::ClearBiblePresentation()
{
	auto removeItem = [](obs_sceneitem_t *&item) {
		if (item) {
			obs_sceneitem_remove(item);
			item = nullptr;
		}
	};
	removeItem(bibleReferenceItem);
	removeItem(bibleVerseItem);
	removeItem(bibleBackgroundItem);
	if (bibleBackgroundSource)
		obs_source_media_stop(bibleBackgroundSource);
	bibleReferenceSource = nullptr;
	bibleVerseSource = nullptr;
	bibleBackgroundSource = nullptr;
}

void PresenterPanel::ProjectBibleVerse(const QString &text, const QString &reference)
{
	if (!stageScene || text.isEmpty() || reference.isEmpty())
		return;
	ClearActiveMedia();
	ClearBiblePresentation();
	activeBibleText = text;
	activeBibleReference = reference;

	const uint32_t canvasWidth = std::max(obs_source_get_width(obs_scene_get_source(stageScene)), 1920u);
	const uint32_t canvasHeight = std::max(obs_source_get_height(obs_scene_get_source(stageScene)), 1080u);
	const char *colorSourceType = obs_get_latest_input_type_id("color_source");
	const char *textSourceType = obs_get_latest_input_type_id("text_gdiplus");
	if (!colorSourceType || !textSourceType)
		return;

	OBSDataAutoRelease backgroundSettings = obs_data_create();
	const QFileInfo backgroundFile(bibleBackgroundPath);
	const QString backgroundSuffix = backgroundFile.suffix().toLower();
	const bool backgroundIsImage =
		backgroundFile.exists() && imageExtensions.contains(backgroundSuffix);
	const bool backgroundIsVideo =
		backgroundFile.exists() && videoExtensions.contains(backgroundSuffix);
	const char *backgroundSourceType = colorSourceType;
	if (backgroundIsImage) {
		backgroundSourceType = obs_get_latest_input_type_id("image_source");
		obs_data_set_string(backgroundSettings, "file", backgroundFile.absoluteFilePath().toUtf8().constData());
	} else if (backgroundIsVideo) {
		backgroundSourceType = obs_get_latest_input_type_id("ffmpeg_source");
		obs_data_set_bool(backgroundSettings, "is_local_file", true);
		obs_data_set_string(backgroundSettings, "local_file",
				    backgroundFile.absoluteFilePath().toUtf8().constData());
		obs_data_set_bool(backgroundSettings, "restart_on_activate", false);
		obs_data_set_bool(backgroundSettings, "close_when_inactive", false);
		obs_data_set_bool(backgroundSettings, "clear_on_media_end", false);
		obs_data_set_bool(backgroundSettings, "looping", bibleBackgroundLoop);
		obs_data_set_bool(backgroundSettings, "full_decode", false);
	} else {
		obs_data_set_int(backgroundSettings, "width", canvasWidth);
		obs_data_set_int(backgroundSettings, "height", canvasHeight);
		obs_data_set_int(backgroundSettings, "color", 0xFF000000);
	}
	if (!backgroundSourceType)
		return;
	OBSSourceAutoRelease background =
		obs_source_create_private(backgroundSourceType, "Presenter Bible Background", backgroundSettings);
	bibleBackgroundSource = background.Get();
	if (backgroundIsVideo && bibleBackgroundSource)
		obs_source_set_volume(bibleBackgroundSource, 0.0f);

	auto createTextSource = [this, textSourceType](OBSSource &target, const QString &sourceName,
						      const QString &content, int fontSize, int width, int height,
						      const QString &alignment) {
		OBSDataAutoRelease settings = obs_data_create();
		OBSDataAutoRelease font = obs_data_create();
		obs_data_set_string(font, "face", bibleFontFamily.toUtf8().constData());
		obs_data_set_string(font, "style", "Regular");
		obs_data_set_int(font, "size", fontSize);
		obs_data_set_int(font, "flags", 0);
		obs_data_set_obj(settings, "font", font);
		obs_data_set_string(settings, "text", content.toUtf8().constData());
		obs_data_set_int(settings, "color", 0xFFFFFF);
		obs_data_set_int(settings, "opacity", 100);
		obs_data_set_string(settings, "align", alignment.toUtf8().constData());
		obs_data_set_string(settings, "valign", "center");
		obs_data_set_bool(settings, "extents", true);
		obs_data_set_bool(settings, "extents_wrap", true);
		obs_data_set_int(settings, "extents_cx", width);
		obs_data_set_int(settings, "extents_cy", height);
		OBSSourceAutoRelease source =
			obs_source_create_private(textSourceType, sourceName.toUtf8().constData(), settings);
		target = source.Get();
	};

	const bool referenceAtTop = bibleReferencePosition.startsWith(QStringLiteral("top"));
	QString referenceAlignment = QStringLiteral("center");
	if (bibleReferencePosition.endsWith(QStringLiteral("left")))
		referenceAlignment = QStringLiteral("left");
	else if (bibleReferencePosition.endsWith(QStringLiteral("right")))
		referenceAlignment = QStringLiteral("right");
	const int verseWidth = static_cast<int>(canvasWidth * 0.82);
	const int verseHeight = static_cast<int>(canvasHeight * 0.66);
	const int referenceWidth = static_cast<int>(canvasWidth * 0.82);
	const int referenceHeight = static_cast<int>(canvasHeight * 0.10);
	createTextSource(bibleVerseSource, QStringLiteral("Presenter Bible Verse"), text, bibleFontSize, verseWidth,
			 verseHeight, bibleTextAlignment);
	createTextSource(bibleReferenceSource, QStringLiteral("Presenter Bible Reference"), reference,
			 std::max(18, int(bibleFontSize * 0.55)), referenceWidth, referenceHeight,
			 referenceAlignment);
	if (!bibleBackgroundSource || !bibleVerseSource || !bibleReferenceSource) {
		ClearBiblePresentation();
		return;
	}

	bibleBackgroundItem = obs_scene_add(stageScene, bibleBackgroundSource);
	bibleVerseItem = obs_scene_add(stageScene, bibleVerseSource);
	bibleReferenceItem = obs_scene_add(stageScene, bibleReferenceSource);
	if (!bibleBackgroundItem || !bibleVerseItem || !bibleReferenceItem) {
		ClearBiblePresentation();
		return;
	}
	const struct vec2 canvasSize = {static_cast<float>(canvasWidth), static_cast<float>(canvasHeight)};
	const struct vec2 canvasCenter = {canvasWidth / 2.0f, canvasHeight / 2.0f};
	obs_sceneitem_set_alignment(bibleBackgroundItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds_alignment(bibleBackgroundItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds_type(bibleBackgroundItem,
				      backgroundIsImage || backgroundIsVideo ? OBS_BOUNDS_SCALE_OUTER
									   : OBS_BOUNDS_STRETCH);
	obs_sceneitem_set_bounds(bibleBackgroundItem, &canvasSize);
	obs_sceneitem_set_pos(bibleBackgroundItem, &canvasCenter);
	const struct vec2 versePosition = {canvasWidth * 0.09f, canvasHeight * (referenceAtTop ? 0.20f : 0.08f)};
	const struct vec2 referencePosition = {canvasWidth * 0.09f, canvasHeight * (referenceAtTop ? 0.06f : 0.84f)};
	obs_sceneitem_set_pos(bibleVerseItem, &versePosition);
	obs_sceneitem_set_pos(bibleReferenceItem, &referencePosition);
	if (backgroundIsVideo)
		obs_source_media_restart(bibleBackgroundSource);
	if (mediaList)
		mediaList->clearSelection();
	currentMedia->setText(reference);
	RefreshTimeline();
}

void PresenterPanel::ActivateMedia(MediaEntry *entry)
{
	if (!entry || !stageScene || !EnsureSource(entry))
		return;
	LoadThumbnail(entry);
	ClearBiblePresentation();
	activeBibleText.clear();
	activeBibleReference.clear();
	ClearActiveMedia(entry);
	activeEntry = entry;
	activeSource = entry->source;
	cachedDuration = 0;
	pendingSeekValue = -1;
	activeItem = obs_scene_add(stageScene, activeSource);
	if (!activeItem)
		return;
	obs_sceneitem_set_bounds_alignment(activeItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_alignment(activeItem, OBS_ALIGN_CENTER);
	const struct vec2 size = {(float)obs_source_get_width(obs_scene_get_source(stageScene)), (float)obs_source_get_height(obs_scene_get_source(stageScene))};
	const struct vec2 position = {size.x / 2.0f, size.y / 2.0f};
	obs_sceneitem_set_bounds(activeItem, &size);
	obs_sceneitem_set_pos(activeItem, &position);
	ApplyActiveItemBounds();
	if (loopButton)
		loopButton->setEnabled(!entry->isImage);
	ApplyAudioSettings();
	if (audioMeter)
		obs_volmeter_attach_source(audioMeter, activeSource);
	signal_handler_disconnect(obs_source_get_signal_handler(activeSource), "media_started", MediaStarted, this);
	signal_handler_connect(obs_source_get_signal_handler(activeSource), "media_started", MediaStarted, this);
	obs_source_media_restart(activeSource);
	RebuildAudioMonitor(true);
	OBSSource monitoredSource = activeSource;
	QTimer::singleShot(500, this, [this, monitoredSource]() {
		if (activeSource.Get() == monitoredSource.Get())
			RebuildAudioMonitor();
	});
	if (!entry->isImage && !entry->thumbnailView) {
		entry->thumbnailView = App()->thumbnails()->createView(mediaList, entry->source);
		connect(entry->thumbnailView, &ThumbnailView::updated, mediaList,
			[this, entry](const QPixmap &pixmap) {
				SetCardThumbnail(entry, pixmap);
				entry->thumbnailLoaded = true;
			});
		entry->thumbnailView->requestUpdate();
	}
	mediaList->setCurrentItem(entry->item);
	currentMedia->setText(QFileInfo(entry->path).fileName());
	RefreshTimeline();
}

void PresenterPanel::ApplyLoopSetting()
{
	if (!activeSource || !activeEntry || activeEntry->isImage)
		return;
	OBSDataAutoRelease settings = obs_source_get_settings(activeSource);
	obs_data_set_bool(settings, "looping", loopCurrent);
	obs_source_update(activeSource, settings);
}

void PresenterPanel::ApplyActiveItemBounds()
{
	if (!activeItem)
		return;
	const QString suffix = activeEntry ? QFileInfo(activeEntry->path).suffix().toLower() : QString();
	const bool visualContent = activeEntry && !audioExtensions.contains(suffix);
	obs_sceneitem_set_bounds_type(activeItem,
				      fitContentToScreen && visualContent ? OBS_BOUNDS_STRETCH : OBS_BOUNDS_SCALE_INNER);
	obs_sceneitem_force_update_transform(activeItem);
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
			playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
		return;
	}
	const int64_t observedDuration = obs_source_media_get_duration(activeSource);
	if (observedDuration > 0)
		cachedDuration = observedDuration;
	const int64_t duration = cachedDuration;
	const int64_t time = obs_source_media_get_time(activeSource);
	const obs_media_state state = obs_source_media_get_state(activeSource);
	if (playPauseButton)
		playPauseButton->setIcon(style()->standardIcon(state == OBS_MEDIA_STATE_PLAYING ? QStyle::SP_MediaPause
										 : QStyle::SP_MediaPlay));
	timelineSlider->setEnabled(duration > 0);
	const bool seekPending = pendingSeekValue >= 0 && QDateTime::currentMSecsSinceEpoch() < seekGuardUntil;
	if (seekPending) {
		timelineSlider->setValue(pendingSeekValue);
	} else if (!timelineDragging && duration > 0) {
		pendingSeekValue = -1;
		timelineSlider->setValue(int(std::clamp<int64_t>(time * 1000 / duration, 0, 1000)));
	}
	timeLabel->setText(QString("%1 / %2").arg(FormatTime(time), FormatTime(duration)));
}

void PresenterPanel::SeekToPendingPosition()
{
	if (!activeSource || pendingSeekValue < 0)
		return;
	const int64_t observedDuration = obs_source_media_get_duration(activeSource);
	if (observedDuration > 0)
		cachedDuration = observedDuration;
	const int64_t duration = cachedDuration;
	if (duration <= 0)
		return;
	obs_source_media_set_time_immediate(activeSource, duration * pendingSeekValue / 1000);
	seekGuardUntil = QDateTime::currentMSecsSinceEpoch() + 1200;
}

void PresenterPanel::RebuildAudioMonitor(bool resetDevice)
{
	if (!activeSource)
		return;
	if (resetDevice && !audioDeviceId.isEmpty()) {
		const char *runtimeName = nullptr;
		const char *runtimeId = nullptr;
		obs_get_audio_monitoring_device(&runtimeName, &runtimeId);
		if (audioDeviceId == QString::fromUtf8(runtimeId ? runtimeId : ""))
			obs_set_audio_monitoring_device("Default", "default");
		obs_set_audio_monitoring_device(audioDeviceName.toUtf8().constData(), audioDeviceId.toUtf8().constData());
	}
	obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_NONE);
	obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_MONITOR_ONLY);
	obs_reset_audio_monitoring();
}

void PresenterPanel::MediaStarted(void *data, calldata_t *)
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel)
		return;
	QMetaObject::invokeMethod(panel, [panel]() { panel->RebuildAudioMonitor(); }, Qt::QueuedConnection);
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
	QList<QListWidgetItem *> visible;
	for (int row = 0; row < mediaList->count(); ++row) {
		if (!mediaList->item(row)->isHidden())
			visible.push_back(mediaList->item(row));
	}
	if (visible.isEmpty())
		return;
	const int current = visible.indexOf(mediaList->currentItem());
	QListWidgetItem *item = visible[(current + 1) % visible.size()];
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
	QList<QListWidgetItem *> visible;
	for (int row = 0; row < mediaList->count(); ++row) {
		if (!mediaList->item(row)->isHidden())
			visible.push_back(mediaList->item(row));
	}
	if (visible.isEmpty())
		return;
	const int current = visible.indexOf(mediaList->currentItem());
	const int previous = current < 0 ? visible.size() - 1 : (current - 1 + visible.size()) % visible.size();
	QListWidgetItem *item = visible[previous];
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
	const char *runtimeDeviceName = nullptr;
	const char *runtimeDeviceId = nullptr;
	obs_get_audio_monitoring_device(&runtimeDeviceName, &runtimeDeviceId);
	const QString currentRuntimeId = QString::fromUtf8(runtimeDeviceId ? runtimeDeviceId : "");
	QDialog dialog(main);
	dialog.setWindowTitle(tr("Configuración de sonido"));
	dialog.setMinimumWidth(500);
	auto *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel(tr("Altavoces / monitor de salida"), &dialog));
	auto *deviceCombo = new QComboBox(&dialog);
	for (const AudioDevice &device : devices) {
		deviceCombo->addItem(device.name, device.id);
		if (device.id == (audioDeviceId.isEmpty() ? currentRuntimeId : audioDeviceId))
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
	const QString newDeviceName = deviceCombo->currentText();
	const QString newDeviceId = deviceCombo->currentData().toString();
	if (!newDeviceId.isEmpty() &&
	    !obs_set_audio_monitoring_device(newDeviceName.toUtf8().constData(), newDeviceId.toUtf8().constData())) {
		QMessageBox::warning(main, tr("Salida de audio"),
				     tr("OPBS no pudo abrir los altavoces seleccionados. Comprueba que sigan conectados."));
		return;
	}
	audioDeviceName = newDeviceName;
	audioDeviceId = newDeviceId;
	RebuildAudioMonitor();
	outputVolume = volume->value();
	outputGain = gain->value();
	selectedEffect = effect->currentData().toString();
	ApplyAudioSettings();
	SaveSettings();
}

void PresenterPanel::ShowBibleDialog()
{
	QDialog dialog(main);
	dialog.setWindowTitle(tr("Configuración de Biblia"));
	dialog.setMinimumSize(620, 480);
	const QRect available = main->screen() ? main->screen()->availableGeometry()
					      : QGuiApplication::primaryScreen()->availableGeometry();
	dialog.resize(std::min(760, std::max(620, available.width() - 80)),
		      std::min(820, std::max(480, available.height() - 80)));
	auto *layout = new QVBoxLayout(&dialog);
	auto *scrollArea = new QScrollArea(&dialog);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	auto *content = new QWidget(scrollArea);
	auto *contentLayout = new QVBoxLayout(content);

	auto *importGroup = new QGroupBox(tr("Agregar Biblia"), content);
	auto *importLayout = new QVBoxLayout(importGroup);
	importLayout->addWidget(new QLabel(
		tr("Selecciona un TXT con bloques [VErsiculo], texto, [referencia] y referencia."), importGroup));
	auto *pathLayout = new QHBoxLayout();
	auto *pathEdit = new QLineEdit(importGroup);
	pathEdit->setPlaceholderText(tr("Ruta del archivo .txt"));
	auto *browseButton = new QPushButton(tr("Examinar…"), importGroup);
	pathLayout->addWidget(pathEdit, 1);
	pathLayout->addWidget(browseButton);
	importLayout->addLayout(pathLayout);
	auto *addButton = new QPushButton(tr("Agregar Biblia"), importGroup);
	importLayout->addWidget(addButton, 0, Qt::AlignRight);
	contentLayout->addWidget(importGroup);

	auto *layoutGroup = new QGroupBox(tr("Layout / lienzo de proyección"), content);
	auto *canvasLayout = new QVBoxLayout(layoutGroup);
	auto *backgroundLabel = new QLabel(tr("Fondo personalizado (imagen o video)"), layoutGroup);
	canvasLayout->addWidget(backgroundLabel);
	auto *backgroundRow = new QHBoxLayout();
	auto *backgroundEdit = new QLineEdit(layoutGroup);
	backgroundEdit->setPlaceholderText(tr("Sin fondo personalizado"));
	backgroundEdit->setText(bibleBackgroundPath);
	auto *backgroundBrowse = new QPushButton(tr("Examinar…"), layoutGroup);
	auto *backgroundClear = new QPushButton(tr("Quitar"), layoutGroup);
	backgroundRow->addWidget(backgroundEdit, 1);
	backgroundRow->addWidget(backgroundBrowse);
	backgroundRow->addWidget(backgroundClear);
	canvasLayout->addLayout(backgroundRow);
	auto *backgroundLoop = new QCheckBox(tr("Repetir el video de fondo en bucle"), layoutGroup);
	backgroundLoop->setChecked(bibleBackgroundLoop);
	canvasLayout->addWidget(backgroundLoop);
	auto *layoutPreview = new BibleLayoutPreview(layoutGroup);
	canvasLayout->addWidget(layoutPreview);
	auto *form = new QFormLayout();
	auto *fontCombo = new QFontComboBox(layoutGroup);
	fontCombo->setCurrentFont(QFont(bibleFontFamily));
	auto *fontSize = new QSpinBox(layoutGroup);
	fontSize->setRange(24, 180);
	fontSize->setSuffix(tr(" px"));
	fontSize->setValue(bibleFontSize);
	auto *alignment = new QComboBox(layoutGroup);
	alignment->addItem(tr("Izquierda"), QStringLiteral("left"));
	alignment->addItem(tr("Centro"), QStringLiteral("center"));
	alignment->addItem(tr("Derecha"), QStringLiteral("right"));
	alignment->setCurrentIndex(std::max(0, alignment->findData(bibleTextAlignment)));
	auto *referencePosition = new QComboBox(layoutGroup);
	referencePosition->addItem(tr("Izquierda superior"), QStringLiteral("top-left"));
	referencePosition->addItem(tr("Centro superior"), QStringLiteral("top-center"));
	referencePosition->addItem(tr("Derecha superior"), QStringLiteral("top-right"));
	referencePosition->addItem(tr("Izquierda inferior"), QStringLiteral("bottom-left"));
	referencePosition->addItem(tr("Centro inferior"), QStringLiteral("bottom-center"));
	referencePosition->addItem(tr("Derecha inferior"), QStringLiteral("bottom-right"));
	referencePosition->setCurrentIndex(std::max(0, referencePosition->findData(bibleReferencePosition)));
	form->addRow(tr("Tipo de letra"), fontCombo);
	form->addRow(tr("Tamaño del versículo"), fontSize);
	form->addRow(tr("Alineación del versículo"), alignment);
	form->addRow(tr("Posición de la referencia"), referencePosition);
	canvasLayout->addLayout(form);
	contentLayout->addWidget(layoutGroup);
	contentLayout->addStretch();
	scrollArea->setWidget(content);
	layout->addWidget(scrollArea, 1);

	auto refreshPreview = [layoutPreview, fontCombo, fontSize, alignment, referencePosition, backgroundEdit,
			       backgroundLoop]() {
		const QString suffix = QFileInfo(backgroundEdit->text().trimmed()).suffix().toLower();
		backgroundLoop->setVisible(videoExtensions.contains(suffix));
		layoutPreview->SetLayout(fontCombo->currentFont().family(), fontSize->value(),
					 alignment->currentData().toString(),
					 referencePosition->currentData().toString(),
					 backgroundEdit->text().trimmed());
	};
	refreshPreview();
	connect(fontCombo, &QFontComboBox::currentFontChanged, &dialog, [refreshPreview](const QFont &) {
		refreshPreview();
	});
	connect(fontSize, qOverload<int>(&QSpinBox::valueChanged), &dialog, [refreshPreview](int) {
		refreshPreview();
	});
	connect(alignment, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [refreshPreview](int) {
		refreshPreview();
	});
	connect(referencePosition, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
		[refreshPreview](int) { refreshPreview(); });
	connect(backgroundEdit, &QLineEdit::textChanged, &dialog, [refreshPreview]() { refreshPreview(); });
	connect(backgroundBrowse, &QPushButton::clicked, &dialog, [this, backgroundEdit]() {
		const QString filter =
			tr("Fondos compatibles (*.bmp *.gif *.jpeg *.jpg *.png *.tga *.webp *.mp4 *.m4v *.mov *.mkv "
			   "*.avi *.webm *.wmv *.mpeg *.mpg)");
		const QString path = QFileDialog::getOpenFileName(main, tr("Seleccionar fondo bíblico"), QString(),
								  filter);
		if (!path.isEmpty())
			backgroundEdit->setText(path);
	});
	connect(backgroundClear, &QPushButton::clicked, &dialog, [backgroundEdit]() { backgroundEdit->clear(); });
	connect(browseButton, &QPushButton::clicked, &dialog, [this, pathEdit]() {
		const QString path = QFileDialog::getOpenFileName(main, tr("Seleccionar Biblia"), QString(),
								  tr("Archivos de texto (*.txt)"));
		if (!path.isEmpty())
			pathEdit->setText(path);
	});
	connect(addButton, &QPushButton::clicked, &dialog, [this, pathEdit]() {
		const QString path = pathEdit->text().trimmed();
		if (path.isEmpty()) {
			QMessageBox::warning(main, tr("Agregar Biblia"), tr("Selecciona primero un archivo TXT."));
			return;
		}
		if (ImportBibleFile(path))
			pathEdit->clear();
	});

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (dialog.exec() != QDialog::Accepted)
		return;

	bibleFontFamily = fontCombo->currentFont().family();
	bibleFontSize = fontSize->value();
	bibleTextAlignment = alignment->currentData().toString();
	bibleReferencePosition = referencePosition->currentData().toString();
	bibleBackgroundPath = backgroundEdit->text().trimmed();
	bibleBackgroundLoop = backgroundLoop->isChecked();
	SaveSettings();
	if (!activeBibleText.isEmpty() && !activeBibleReference.isEmpty())
		ProjectBibleVerse(activeBibleText, activeBibleReference);
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

QString PresenterPanel::SelectedFolderId() const
{
	if (folderList && folderList->currentItem())
		return folderList->currentItem()->data(Qt::UserRole).toString();
	if (presentationFolderList && presentationFolderList->currentItem())
		return presentationFolderList->currentItem()->data(Qt::UserRole).toString();
	return currentFolderId.isEmpty() ? QStringLiteral("general") : currentFolderId;
}

void PresenterPanel::ApplyLibraryFilter()
{
	if (!mediaList)
		return;
	const QString folderId = SelectedFolderId();
	const bool bibleMode = folderId == QString::fromLatin1(kBibleFolderId);
	if (bibleControls)
		bibleControls->setVisible(bibleMode);
	if (bibleResultsList)
		bibleResultsList->setVisible(bibleMode);
	if (searchEdit)
		searchEdit->setVisible(!bibleMode);
	if (bibleMode) {
		mediaList->hide();
		ApplyBibleFilter();
		return;
	}
	if (bibleResultsList)
		bibleResultsList->hide();
	const QString query = searchEdit ? searchEdit->text().trimmed() : QString();
	int visible = 0;
	for (const auto &entry : entries) {
		const bool matchesFolder = entry->folderId == folderId;
		const bool matchesSearch = query.isEmpty() ||
			QFileInfo(entry->path).fileName().contains(query, Qt::CaseInsensitive);
		const bool show = matchesFolder && matchesSearch;
		if (entry->item)
			entry->item->setHidden(!show);
		if (show)
			++visible;
	}
	mediaCount->setText(tr("%1 archivos").arg(visible));
	mediaList->setVisible(visible > 0);
	emptyState->setVisible(visible == 0);
	emptyState->setText(query.isEmpty() ? tr("Arrastra aquí imágenes, videos o audio para comenzar")
					 : tr("No hay resultados en esta carpeta"));
	QTimer::singleShot(0, this, &PresenterPanel::LoadVisibleThumbnails);
}

void PresenterPanel::ApplyBibleFilter()
{
	if (!bibleResultsList || SelectedFolderId() != QString::fromLatin1(kBibleFolderId))
		return;
	bibleResultsList->clear();
	const QString query = NormalizeBibleText(bibleSearchEdit ? bibleSearchEdit->text() : QString());
	if (currentBiblePath.isEmpty() || bibleVerses.empty()) {
		mediaCount->setText(tr("0 versículos"));
		emptyState->setText(tr("No hay biblias cargadas"));
		emptyState->show();
		bibleResultsList->hide();
		return;
	}
	if (query.isEmpty()) {
		mediaCount->setText(tr("%1 versículos").arg(bibleVerses.size()));
		emptyState->setText(tr("Escribe una palabra, frase o referencia para buscar en la Biblia"));
		emptyState->show();
		bibleResultsList->hide();
		return;
	}

	int matches = 0;
	for (const BibleVerse &verse : bibleVerses) {
		if (!verse.searchableText.contains(query))
			continue;
		++matches;
		if (bibleResultsList->count() >= kBibleResultLimit)
			continue;
		auto *item = new QListWidgetItem(QStringLiteral("%1\n\n%2").arg(verse.reference, verse.text),
						 bibleResultsList);
		item->setToolTip(QStringLiteral("%1\n\n%2").arg(verse.reference, verse.text));
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
		item->setData(Qt::UserRole, verse.reference);
		item->setData(Qt::UserRole + 1, verse.text);
		item->setSizeHint(QSize(300, 155));
	}
	if (matches > kBibleResultLimit)
		mediaCount->setText(tr("%1 de %2 coincidencias").arg(kBibleResultLimit).arg(matches));
	else
		mediaCount->setText(tr("%1 coincidencias").arg(matches));
	emptyState->setText(tr("No se encontraron versículos"));
	emptyState->setVisible(matches == 0);
	bibleResultsList->setVisible(matches > 0);
}

QString PresenterPanel::BibleDirectoryPath() const
{
	BPtr<char> path(GetAppConfigPathPtr("obs-studio/bibles"));
	return QString::fromUtf8(path.Get());
}

QString PresenterPanel::PresentationsDirectoryPath() const
{
	BPtr<char> path(GetAppConfigPathPtr("obs-studio/presentations"));
	return QString::fromUtf8(path.Get());
}

void PresenterPanel::LoadBibleCatalog(const QString &preferredPath)
{
	if (!bibleSelector)
		return;
	const QSignalBlocker blocker(bibleSelector);
	bibleSelector->clear();
	const QString directoryPath = BibleDirectoryPath();
	QDir().mkpath(directoryPath);
	const QDir directory(directoryPath);
	const QFileInfoList files = directory.entryInfoList({"*.txt"}, QDir::Files | QDir::Readable, QDir::Name);
	for (const QFileInfo &file : files)
		bibleSelector->addItem(BibleDisplayName(file.absoluteFilePath()), file.absoluteFilePath());

	int selectedIndex = 0;
	if (!preferredPath.isEmpty()) {
		for (int index = 0; index < bibleSelector->count(); ++index) {
			if (QFileInfo(bibleSelector->itemData(index).toString()).absoluteFilePath() ==
			    QFileInfo(preferredPath).absoluteFilePath()) {
				selectedIndex = index;
				break;
			}
		}
	}
	if (bibleSelector->count() > 0)
		bibleSelector->setCurrentIndex(selectedIndex);
	LoadBibleTranslation(bibleSelector->currentIndex());
}

void PresenterPanel::LoadBibleTranslation(int index)
{
	bibleVerses.clear();
	currentBiblePath.clear();
	if (!bibleSelector || index < 0 || index >= bibleSelector->count())
		return;
	const QString path = bibleSelector->itemData(index).toString();
	std::vector<BibleVerse> parsedVerses;
	if (!ParseBibleFile(path, parsedVerses))
		return;
	currentBiblePath = path;
	bibleVerses = std::move(parsedVerses);
}

bool PresenterPanel::ParseBibleFile(const QString &path, std::vector<BibleVerse> &verses, QString *error) const
{
	verses.clear();
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		if (error)
			*error = tr("No se pudo abrir el archivo.");
		return false;
	}
	QTextStream stream(&file);
	stream.setEncoding(QStringConverter::Utf8);
	enum class Section { None, Verse, Reference };
	Section section = Section::None;
	QStringList verseLines;
	QStringList referenceLines;
	bool invalid = false;
	auto finishVerse = [this, &verses, &verseLines, &referenceLines, &invalid]() {
		const QString text = verseLines.join(' ').simplified();
		const QString reference = referenceLines.join(' ').simplified();
		if (text.isEmpty() || reference.isEmpty()) {
			invalid = true;
		} else {
			verses.push_back({text, reference, NormalizeBibleText(reference + ' ' + text)});
		}
		verseLines.clear();
		referenceLines.clear();
	};

	while (!stream.atEnd()) {
		const QString line = stream.readLine().trimmed();
		if (line.compare(QStringLiteral("[VErsiculo]"), Qt::CaseInsensitive) == 0) {
			if (section != Section::None) {
				invalid = true;
				break;
			}
			section = Section::Verse;
		} else if (line.compare(QStringLiteral("[referencia]"), Qt::CaseInsensitive) == 0) {
			if (section != Section::Verse || verseLines.isEmpty()) {
				invalid = true;
				break;
			}
			section = Section::Reference;
		} else if (line.startsWith(QStringLiteral("---"))) {
			if (section == Section::Reference) {
				finishVerse();
			} else if (section != Section::None) {
				invalid = true;
				break;
			}
			section = Section::None;
		} else if (!line.isEmpty()) {
			if (section == Section::Verse)
				verseLines.push_back(line);
			else if (section == Section::Reference)
				referenceLines.push_back(line);
			else {
				invalid = true;
				break;
			}
		}
	}
	if (!invalid && section == Section::Reference)
		finishVerse();
	else if (!invalid && section != Section::None)
		invalid = true;
	if (invalid || verses.empty()) {
		verses.clear();
		if (error)
			*error = tr("El TXT no sigue el formato [VErsiculo], texto, [referencia] y referencia.");
		return false;
	}
	return true;
}

bool PresenterPanel::ImportBibleFile(const QString &path)
{
	std::vector<BibleVerse> parsedVerses;
	QString error;
	if (!ParseBibleFile(path, parsedVerses, &error)) {
		QMessageBox::critical(main, tr("Biblia incompatible"),
				      tr("Biblia incompatible.\n\n%1").arg(error));
		return false;
	}
	const QFileInfo source(path);
	const QString directoryPath = BibleDirectoryPath();
	QDir().mkpath(directoryPath);
	const QString destination = QDir(directoryPath).filePath(source.fileName());
	if (QFileInfo(source.absoluteFilePath()).absoluteFilePath() != QFileInfo(destination).absoluteFilePath()) {
		QFile input(source.absoluteFilePath());
		if (!input.open(QIODevice::ReadOnly)) {
			QMessageBox::critical(main, tr("Agregar Biblia"), tr("No se pudo leer el archivo seleccionado."));
			return false;
		}
		QSaveFile output(destination);
		if (!output.open(QIODevice::WriteOnly) || output.write(input.readAll()) < 0 || !output.commit()) {
			QMessageBox::critical(main, tr("Agregar Biblia"),
					      tr("No se pudo guardar la Biblia en la aplicación."));
			return false;
		}
	}
	LoadBibleCatalog(destination);
	ApplyBibleFilter();
	SaveSettings();
	QMessageBox::information(main, tr("Agregar Biblia"),
				 tr("La Biblia “%1” se agregó correctamente.").arg(BibleDisplayName(destination)));
	return true;
}

void PresenterPanel::CreateFolder()
{
	bool accepted = false;
	const QString name = QInputDialog::getText(this, tr("Nueva carpeta"), tr("Nombre de la carpeta:"),
					     QLineEdit::Normal, tr("Nueva carpeta"), &accepted)
				     .trimmed();
	if (!accepted || name.isEmpty())
		return;
	const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), name, folderList);
	item->setData(Qt::UserRole, id);
	item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
	folders.push_back({id, name});
	folderList->setCurrentItem(item);
	SaveSettings();
}

void PresenterPanel::RenameFolder()
{
	if (folderList && folderList->currentItem())
		folderList->editItem(folderList->currentItem());
}

void PresenterPanel::MoveMediaToFolder(const QStringList &paths, const QString &folderId)
{
	for (const QString &path : paths) {
		for (const auto &entry : entries) {
			if (QFileInfo(entry->path).absoluteFilePath() == QFileInfo(path).absoluteFilePath()) {
				entry->folderId = folderId;
				break;
			}
		}
	}
	ApplyLibraryFilter();
	SaveSettings();
}

void PresenterPanel::RemoveMediaEntry(MediaEntry *entry)
{
	if (!entry)
		return;
	auto found = std::find_if(entries.begin(), entries.end(),
				 [entry](const auto &candidate) { return candidate.get() == entry; });
	if (found == entries.end())
		return;
	const bool wasActive = activeEntry == entry;
	if (wasActive) {
		ClearActiveMedia();
		currentMedia->setText(tr("Ningún contenido seleccionado"));
	} else {
		ReleaseSource(entry);
	}
	if (entry->thumbnailView)
		delete entry->thumbnailView.data();
	entry->thumbnailView = nullptr;
	delete entry->item;
	entry->item = nullptr;
	entries.erase(found);
	ApplyLibraryFilter();
	SaveSettings();
}

void PresenterPanel::ReorderFoldersFromList()
{
	folders.clear();
	for (int row = 0; row < folderList->count(); ++row) {
		QListWidgetItem *item = folderList->item(row);
		folders.push_back({item->data(Qt::UserRole).toString(), item->text()});
	}
	SaveSettings();
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
	loopCurrent = settings.value("playback/loopCurrent", false).toBool();
	fitContentToScreen = settings.value("view/fitContentToScreen", false).toBool();
	bibleFontFamily = settings.value("bible/fontFamily", "Arial").toString();
	bibleFontSize = std::clamp(settings.value("bible/fontSize", 96).toInt(), 24, 180);
	bibleTextAlignment = settings.value("bible/textAlignment", "center").toString();
	bibleReferencePosition = settings.value("bible/referencePosition", "bottom-center").toString();
	bibleBackgroundPath = settings.value("bible/backgroundPath").toString();
	bibleBackgroundLoop = settings.value("bible/backgroundLoop", false).toBool();
	LoadBibleCatalog(settings.value("bible/selectedPath").toString());
	mediaVolumeSlider->setValue(mediaVolume);
	if (loopButton) {
		loopButton->blockSignals(true);
		loopButton->setChecked(loopCurrent);
		loopButton->blockSignals(false);
	}
	if (fitToScreenAction) {
		fitToScreenAction->blockSignals(true);
		fitToScreenAction->setChecked(fitContentToScreen);
		fitToScreenAction->blockSignals(false);
	}
	const QByteArray geometry = settings.value("window/geometry").toByteArray();
	if (!geometry.isEmpty())
		main->restoreGeometry(geometry);
	if (!audioDeviceId.isEmpty())
		obs_set_audio_monitoring_device(audioDeviceName.toUtf8().constData(), audioDeviceId.toUtf8().constData());

	folders.clear();
	folderList->clear();
	const int folderCount = settings.beginReadArray("folders");
	for (int index = 0; index < folderCount; ++index) {
		settings.setArrayIndex(index);
		const QString id = settings.value("id").toString();
		const QString name = settings.value("name").toString();
		if (id.isEmpty() || name.isEmpty())
			continue;
		folders.push_back({id, name});
		auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), name, folderList);
		item->setData(Qt::UserRole, id);
		item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
	}
	settings.endArray();
	if (folders.empty()) {
		folders.push_back({"general", tr("General")});
		auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), tr("General"), folderList);
		item->setData(Qt::UserRole, "general");
		item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
	}
	currentFolderId = settings.value("library/currentFolder", folders.front().id).toString();
	for (int row = 0; row < folderList->count(); ++row) {
		if (folderList->item(row)->data(Qt::UserRole).toString() == currentFolderId) {
			folderList->setCurrentRow(row);
			break;
		}
	}
	if (presentationFolderList) {
		for (int row = 0; row < presentationFolderList->count(); ++row) {
			if (presentationFolderList->item(row)->data(Qt::UserRole).toString() == currentFolderId) {
				presentationFolderList->setCurrentRow(row);
				break;
			}
		}
	}
	if (!folderList->currentItem()) {
		if (!presentationFolderList || !presentationFolderList->currentItem()) {
			folderList->setCurrentRow(0);
			currentFolderId = folderList->currentItem()->data(Qt::UserRole).toString();
		}
	}

	const int mediaEntryCount = settings.beginReadArray("media");
	for (int index = 0; index < mediaEntryCount; ++index) {
		settings.setArrayIndex(index);
		AddMediaFile(settings.value("path").toString(), settings.value("folderId", "general").toString(), false);
	}
	settings.endArray();
	if (mediaEntryCount == 0) {
		for (const QString &path : settings.value("library/files").toStringList())
			AddMediaFile(path, "general", false);
	}
	ApplyLibraryFilter();
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
	settings.remove("folders");
	settings.beginWriteArray("folders", folderList ? folderList->count() : 0);
	if (folderList) {
		for (int index = 0; index < folderList->count(); ++index) {
			settings.setArrayIndex(index);
			settings.setValue("id", folderList->item(index)->data(Qt::UserRole).toString());
			settings.setValue("name", folderList->item(index)->text());
		}
	}
	settings.endArray();
	settings.remove("media");
	settings.beginWriteArray("media", static_cast<int>(entries.size()));
	for (int index = 0; index < static_cast<int>(entries.size()); ++index) {
		settings.setArrayIndex(index);
		settings.setValue("path", entries[index]->path);
		settings.setValue("folderId", entries[index]->folderId);
	}
	settings.endArray();
	settings.remove("library/files");
	settings.setValue("library/currentFolder", SelectedFolderId());
	settings.setValue("stage/monitorName", selectedMonitorName);
	settings.setValue("stage/enabled", stageEnabled);
	settings.setValue("playback/loopCurrent", loopCurrent);
	settings.setValue("view/fitContentToScreen", fitContentToScreen);
	settings.setValue("bible/selectedPath", currentBiblePath);
	settings.setValue("bible/fontFamily", bibleFontFamily);
	settings.setValue("bible/fontSize", bibleFontSize);
	settings.setValue("bible/textAlignment", bibleTextAlignment);
	settings.setValue("bible/referencePosition", bibleReferencePosition);
	settings.setValue("bible/backgroundPath", bibleBackgroundPath);
	settings.setValue("bible/backgroundLoop", bibleBackgroundLoop);
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
