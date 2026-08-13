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
#include <components/VolumeControl.hpp>
#include <utility/ThumbnailManager.hpp>
#include <utility/ThumbnailView.hpp>
#include <utility/display-helpers.hpp>

#include <obs-audio-controls.h>
#include <obs-frontend-api.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
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
#include <QMainWindow>
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
#include <QStackedWidget>
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
constexpr int kThumbnailWidth = 176;
constexpr int kThumbnailHeight = 99;
constexpr int kBibleResultLimit = 250;
constexpr auto kBibleFolderId = "presentation-bible";
constexpr auto kPresentationsFolderId = "presentation-slides";
constexpr auto kCaptureFolderId = "tools-capture";
constexpr auto kNdiFolderId = "tools-ndi";
constexpr auto kTransmissionAudioBridgeId = "opbs_transmission_audio_bridge";
const QStringList imageExtensions = {"bmp", "gif", "jpeg", "jpg", "png", "tga", "webp"};
const QStringList audioExtensions = {"mp3", "aac", "ogg", "wav", "flac", "m4a", "wma"};
const QStringList videoExtensions = {"mp4", "m4v", "mov", "mkv", "avi", "webm", "wmv", "mpeg", "mpg"};

long long ColorToObsInt(const QColor &color)
{
	return (color.red() & 0xff) | ((color.green() & 0xff) << 8) | ((color.blue() & 0xff) << 16) |
	       ((color.alpha() & 0xff) << 24);
}

const char *TransmissionAudioBridgeName(void *)
{
	return "OPBS Transmission Presenter Audio";
}

void *CreateTransmissionAudioBridge(obs_data_t *, obs_source_t *)
{
	return new int(0);
}

void DestroyTransmissionAudioBridge(void *data)
{
	delete static_cast<int *>(data);
}

void RegisterTransmissionAudioBridge()
{
	if (obs_get_latest_input_type_id(kTransmissionAudioBridgeId))
		return;
	obs_source_info info = {};
	info.id = kTransmissionAudioBridgeId;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.get_name = TransmissionAudioBridgeName;
	info.create = CreateTransmissionAudioBridge;
	info.destroy = DestroyTransmissionAudioBridge;
	info.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT;
	obs_register_source(&info);
}

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

class AudioPlaylistList final : public QListWidget {
public:
	std::function<void(const QStringList &)> filesDropped;

	explicit AudioPlaylistList(QWidget *parent) : QListWidget(parent)
	{
		setAcceptDrops(true);
		setDropIndicatorShown(true);
		setDragDropMode(QAbstractItemView::DropOnly);
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
		if (!event->mimeData()->hasUrls()) {
			QListWidget::dropEvent(event);
			return;
		}
		QStringList paths;
		for (const QUrl &url : event->mimeData()->urls())
			if (url.isLocalFile())
				paths.push_back(url.toLocalFile());
		if (!paths.isEmpty() && filesDropped)
			filesDropped(paths);
		event->acceptProposedAction();
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
	restoring = true;
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
		#transmissionPreviewFrame { background: #15181f; border: 1px solid #2a2e38; border-radius: 8px; }
		QToolButton#transmissionMode { background: #252a35; color: #f4f6fb; border: 1px solid #373e4c;
			border-radius: 7px; padding: 7px 10px; font-weight: 600; }
		QToolButton#transmissionMode:hover { border-color: #7b6cff; }
		QToolButton#transmissionMode:checked { background: #6d5dfc; border-color: #8d82ff; }
		QToolButton#transmissionLive { background: #b62f3b; color: white; border: 0; border-radius: 7px;
			padding: 7px 12px; font-weight: 700; }
		QToolButton#transmissionLive:checked { background: #e04a58; }
	)" );

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);
	auto *header = new QFrame(this);
	header->setObjectName("presenterHeader");
	header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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

	dockWorkspace = new QMainWindow(this);
	dockWorkspace->setWindowFlags(Qt::Widget);
	dockWorkspace->setObjectName("presenterDockWorkspace");
	dockWorkspace->setDockNestingEnabled(true);
	dockWorkspace->setAnimated(false);
	dockWorkspace->setMinimumHeight(420);
	auto *dockAnchor = new QWidget(dockWorkspace);
	dockAnchor->setFixedSize(1, 1);
	dockWorkspace->setCentralWidget(dockAnchor);
	root->addWidget(dockWorkspace, 1);
	auto *previewFrame = new QFrame(dockWorkspace);
	previewFrame->setObjectName("presenterPreviewFrame");
	previewFrame->setMinimumWidth(420);
	auto *previewLayout = new QVBoxLayout(previewFrame);
	previewLayout->setSizeConstraint(QLayout::SetNoConstraint);
	previewLayout->setContentsMargins(16, 16, 16, 16);
	auto *live = new QLabel(tr("●  VISTA PREVIA EN VIVO"), previewFrame);
	live->setObjectName("presenterLive");
	previewLayout->addWidget(live);
	preview = new OBSQTDisplay(previewFrame);
	preview->setMinimumSize(288, 162);
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
	mediaVolumeSlider->hide();
	audioRow->addWidget(audioLabel);
	audioRow->addLayout(meters, 1);
	previewLayout->addLayout(audioRow);
	connect(mediaVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
		mediaVolume = value;
		ApplyAudioSettings();
		if (!restoring)
			SaveSettings();
	});

	auto *transmissionFrame = new QFrame(dockWorkspace);
	transmissionFrame->setObjectName("transmissionPreviewFrame");
	auto *transmissionLayout = new QVBoxLayout(transmissionFrame);
	transmissionLayout->setContentsMargins(10, 10, 10, 10);
	auto *transmissionLabel = new QLabel(tr("●  VISTA PREVIA DE TRANSMISIÓN"), transmissionFrame);
	transmissionLabel->setObjectName("presenterLive");
	transmissionLayout->addWidget(transmissionLabel);
	transmissionPreview = new OBSQTDisplay(transmissionFrame);
	transmissionPreview->setMinimumSize(288, 162);
	transmissionPreview->SetDisplayBackgroundColor(QColor("#08090c"));
	transmissionLayout->addWidget(transmissionPreview, 1);
	auto *transmissionButtons = new QHBoxLayout();
	transmissionButtons->setSpacing(7);
	auto makeTransmissionButton = [transmissionFrame, transmissionButtons](const QString &text, const QString &tip,
								 bool checkable = true) {
		auto *button = new QToolButton(transmissionFrame);
		button->setObjectName("transmissionMode");
		button->setText(text);
		button->setToolTip(tip);
		button->setCheckable(checkable);
		transmissionButtons->addWidget(button);
		return button;
	};
	streamButton = makeTransmissionButton(tr("Transmitir"), tr("Iniciar o detener los destinos configurados"));
	streamButton->setObjectName("transmissionLive");
	recordButton = makeTransmissionButton(tr("Grabar"), tr("Iniciar o detener la grabación"));
	recordButton->setObjectName("transmissionLive");
	transmissionButtons->addStretch();
	camerasViewButton = makeTransmissionButton(tr("Cámaras"), tr("Mostrar solo la cámara asignada"));
	presenterViewButton = makeTransmissionButton(tr("Presentador"), tr("Mostrar solo el contenido del presentador"));
	combinedViewButton = makeTransmissionButton(tr("Ambos"),
						      tr("Mostrar la cámara con el presentador en recuadro"));
	transmissionLayout->addLayout(transmissionButtons);
	connect(streamButton, &QToolButton::clicked, this, &PresenterPanel::ToggleStreaming);
	connect(recordButton, &QToolButton::clicked, this, &PresenterPanel::ToggleRecording);
	connect(camerasViewButton, &QToolButton::clicked, this,
		[this]() { ApplyTransmissionView(TransmissionView::Cameras); });
	connect(presenterViewButton, &QToolButton::clicked, this,
		[this]() { ApplyTransmissionView(TransmissionView::Presenter); });
	connect(combinedViewButton, &QToolButton::clicked, this,
		[this]() { ApplyTransmissionView(TransmissionView::CamerasAndPresenter); });

	auto *library = new QFrame(dockWorkspace);
	library->setObjectName("presenterLibrary");
	auto *libraryLayout = new QVBoxLayout(library);
	libraryLayout->setContentsMargins(16, 16, 16, 16);
	auto *libraryHead = new QHBoxLayout();
	auto *libraryTitle = new QLabel(tr("MULTIMEDIA"), library);
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
	folderLayout->addWidget(new QLabel(tr("CARPETAS"), folderPanel));
	auto *foldersWidget = new PresenterFolderList(folderPanel);
	folderList = foldersWidget;
	foldersWidget->setObjectName("presenterFolderList");
	foldersWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	foldersWidget->setMinimumHeight(160);
	folderLayout->addWidget(foldersWidget, 1);
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
	folderLayout->addStretch();
	auto *presentationsWidget = new PresenterFolderList(dockWorkspace);
	presentationFolderList = presentationsWidget;
	presentationsWidget->setObjectName("presenterPresentationList");
	presentationsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	presentationsWidget->setDragEnabled(false);
	presentationsWidget->setDragDropMode(QAbstractItemView::DropOnly);
	presentationsWidget->setFixedHeight(190);
	auto addPresentationFolder = [this, presentationsWidget](const QString &id, const QString &name) {
		auto *item = new QListWidgetItem(name, presentationsWidget);
		item->setData(Qt::UserRole, id);
		item->setFlags((item->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsEditable) | Qt::ItemIsDropEnabled);
	};
	addPresentationFolder(QString::fromLatin1(kBibleFolderId), tr("BIBLIA"));
	addPresentationFolder(QString::fromLatin1(kPresentationsFolderId), tr("PRESENTACIÓN"));
	addPresentationFolder(QString::fromLatin1(kCaptureFolderId), tr("CAPTURA"));
	addPresentationFolder(QString::fromLatin1(kNdiFolderId), tr("NDI"));
	libraryBody->addWidget(folderPanel);
	libraryContentHost = new QWidget(library);
	auto *mediaArea = new QVBoxLayout(libraryContentHost);
	mediaArea->setContentsMargins(0, 0, 0, 0);
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
	multimediaContentTarget = new QWidget(library);
	auto *multimediaTargetLayout = new QVBoxLayout(multimediaContentTarget);
	multimediaTargetLayout->setContentsMargins(0, 0, 0, 0);
	multimediaTargetLayout->addWidget(libraryContentHost);
	libraryBody->addWidget(multimediaContentTarget, 1);
	libraryLayout->addLayout(libraryBody, 1);

	connect(addFolder, &QToolButton::clicked, this, &PresenterPanel::CreateFolder);
	connect(renameFolder, &QToolButton::clicked, this, &PresenterPanel::RenameFolder);
	connect(foldersWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
		if (!current)
			return;
		if (presentationFolderList)
			presentationFolderList->setCurrentRow(-1);
		currentFolderId = current->data(Qt::UserRole).toString();
		ShowLibraryContent(false);
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
		ShowLibraryContent(true);
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

	auto *toolsFrame = new QFrame(dockWorkspace);
	toolsFrame->setObjectName("presenterLibrary");
	auto *toolsLayout = new QHBoxLayout(toolsFrame);
	toolsLayout->setContentsMargins(8, 8, 8, 8);
	auto *toolsSidebar = new QFrame(toolsFrame);
	toolsSidebar->setObjectName("presenterFolderPanel");
	toolsSidebar->setMinimumWidth(165);
	toolsSidebar->setMaximumWidth(230);
	auto *toolsSidebarLayout = new QVBoxLayout(toolsSidebar);
	toolsSidebarLayout->setContentsMargins(8, 8, 8, 8);
	auto *toolsTitle = new QLabel(tr("HERRAMIENTAS"), toolsSidebar);
	toolsTitle->setObjectName("presenterCurrent");
	toolsSidebarLayout->addWidget(toolsTitle);
	toolsSidebarLayout->addWidget(presentationsWidget);
	toolsSidebarLayout->addSpacing(12);
	auto *recentTitle = new QLabel(tr("PRESENTACIONES RECIENTES"), toolsSidebar);
	recentTitle->setObjectName("presenterMediaCount");
	toolsSidebarLayout->addWidget(recentTitle);
	recentPresentationsList = new QListWidget(toolsSidebar);
	recentPresentationsList->setObjectName("presenterPresentationList");
	recentPresentationsList->setMaximumHeight(170);
	recentPresentationsList->setToolTip(tr("Se conservan las cuatro presentaciones importadas más recientes"));
	toolsSidebarLayout->addWidget(recentPresentationsList, 1);
	connect(recentPresentationsList, &QListWidget::itemClicked, this,
		[this](QListWidgetItem *item) { ActivateRecentPresentation(recentPresentationsList->row(item)); });
	toolsLayout->addWidget(toolsSidebar);
	toolsContentTarget = new QWidget(toolsFrame);
	toolsContentTarget->setLayout(new QVBoxLayout());
	toolsContentTarget->layout()->setContentsMargins(0, 0, 0, 0);
	toolsLayout->addWidget(toolsContentTarget, 1);

	auto *audioFrame = new QFrame(dockWorkspace);
	audioFrame->setObjectName("presenterLibrary");
	auto *audioPlayerLayout = new QVBoxLayout(audioFrame);
	audioPlayerLayout->setContentsMargins(10, 10, 10, 10);
	auto *audioTitle = new QLabel(tr("REPRODUCTOR DE AUDIO"), audioFrame);
	audioTitle->setObjectName("presenterCurrent");
	audioPlayerLayout->addWidget(audioTitle);
	audioPlayerTimeline = new AbsoluteSlider(Qt::Horizontal, audioFrame);
	audioPlayerTimeline->setRange(0, 1000);
	audioPlayerTimeline->setEnabled(false);
	audioPlayerLayout->addWidget(audioPlayerTimeline);
	auto *audioControls = new QHBoxLayout();
	audioControls->addStretch();
	audioPlayerPlayPauseButton = new QToolButton(audioFrame);
	audioPlayerPlayPauseButton->setObjectName("presenterTransport");
	audioPlayerPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	audioPlayerPlayPauseButton->setToolTip(tr("Reproducir / pausar audio"));
	auto *audioStopButton = new QToolButton(audioFrame);
	audioStopButton->setObjectName("presenterTransport");
	audioStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	audioStopButton->setToolTip(tr("Detener audio"));
	audioControls->addWidget(audioPlayerPlayPauseButton);
	audioControls->addWidget(audioStopButton);
	audioControls->addStretch();
	audioPlayerLayout->addLayout(audioControls);
	audioPlaylistList = new AudioPlaylistList(audioFrame);
	audioPlaylistList->setObjectName("presenterPresentationList");
	audioPlaylistList->setAlternatingRowColors(true);
	audioPlaylistList->setToolTip(tr("Arrastra aquí archivos de audio"));
	audioPlayerLayout->addWidget(audioPlaylistList, 1);
	static_cast<AudioPlaylistList *>(audioPlaylistList.data())->filesDropped =
		[this](const QStringList &paths) { AddAudioPlayerFiles(paths); };
	connect(audioPlaylistList, &QListWidget::itemClicked, this,
		[this](QListWidgetItem *item) { PlayAudioPlayerRow(audioPlaylistList->row(item)); });
	connect(audioPlayerPlayPauseButton, &QToolButton::clicked, this, &PresenterPanel::ToggleAudioPlayer);
	connect(audioStopButton, &QToolButton::clicked, this, &PresenterPanel::StopAudioPlayer);
	connect(audioPlayerTimeline, &QSlider::sliderPressed, this, [this]() { audioPlayerTimelineDragging = true; });
	connect(audioPlayerTimeline, &QSlider::sliderReleased, this, [this]() {
		audioPlayerTimelineDragging = false;
		if (!audioPlayerSource)
			return;
		const int64_t duration = obs_source_media_get_duration(audioPlayerSource);
		if (duration > 0)
			obs_source_media_set_time_immediate(audioPlayerSource,
						    duration * audioPlayerTimeline->value() / 1000);
	});

	auto makeDock = [this](const QString &name, const QString &title, QWidget *widget) {
		auto *dock = new QDockWidget(title, dockWorkspace);
		dock->setObjectName(name);
		dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
		dock->setAllowedAreas(Qt::AllDockWidgetAreas);
		dock->setWidget(widget);
		return dock;
	};
	auto *presenterDock = makeDock("opbsPresenterDock", tr("PRESENTADOR / ESCENARIO"), previewFrame);
	auto *transmissionDock = makeDock("opbsTransmissionDock", tr("TRANSMISIÓN / EN VIVO"), transmissionFrame);
	auto *multimediaDock = makeDock("opbsMultimediaDock", tr("MULTIMEDIA"), library);
	auto *toolsDock = makeDock("opbsToolsDock", tr("HERRAMIENTAS"), toolsFrame);
	auto *audioDock = makeDock("opbsAudioPlayerDock", tr("REPRODUCTOR DE AUDIO"), audioFrame);
	dockWorkspace->addDockWidget(Qt::LeftDockWidgetArea, presenterDock);
	dockWorkspace->addDockWidget(Qt::LeftDockWidgetArea, transmissionDock);
	dockWorkspace->addDockWidget(Qt::LeftDockWidgetArea, multimediaDock);
	dockWorkspace->addDockWidget(Qt::LeftDockWidgetArea, toolsDock);
	dockWorkspace->addDockWidget(Qt::LeftDockWidgetArea, audioDock);
	dockWorkspace->splitDockWidget(presenterDock, transmissionDock, Qt::Vertical);
	dockWorkspace->splitDockWidget(presenterDock, multimediaDock, Qt::Horizontal);
	dockWorkspace->splitDockWidget(transmissionDock, toolsDock, Qt::Horizontal);
	dockWorkspace->splitDockWidget(toolsDock, audioDock, Qt::Horizontal);
	dockWorkspace->resizeDocks({presenterDock, multimediaDock}, {600, 1300}, Qt::Horizontal);
	dockWorkspace->resizeDocks({presenterDock, transmissionDock}, {420, 460}, Qt::Vertical);
	dockWorkspace->resizeDocks({multimediaDock, toolsDock}, {420, 460}, Qt::Vertical);
	dockWorkspace->resizeDocks({toolsDock, audioDock}, {1000, 300}, Qt::Horizontal);
	for (QDockWidget *dock : {presenterDock, transmissionDock, multimediaDock, toolsDock, audioDock}) {
		dock->setFloating(false);
		dock->show();
	}

	timelineTimer = new QTimer(this);
	timelineTimer->setInterval(250);
	connect(timelineTimer, &QTimer::timeout, this, &PresenterPanel::RefreshTimeline);
	seekTimer = new QTimer(this);
	seekTimer->setSingleShot(true);
	connect(seekTimer, &QTimer::timeout, this, &PresenterPanel::SeekToPendingPosition);
	audioPlayerTimer = new QTimer(this);
	audioPlayerTimer->setInterval(250);
	connect(audioPlayerTimer, &QTimer::timeout, this, &PresenterPanel::RefreshAudioPlayerTimeline);
	audioPlayerTimer->start();
}

void PresenterPanel::ShowLibraryContent(bool tools)
{
	if (!libraryContentHost)
		return;
	if (mediaCount)
		mediaCount->setVisible(!tools);
	QWidget *target = tools ? toolsContentTarget.data() : multimediaContentTarget.data();
	if (!target || libraryContentHost->parentWidget() == target)
		return;
	libraryContentHost->setParent(target);
	target->layout()->addWidget(libraryContentHost);
	libraryContentHost->show();
}

void PresenterPanel::AddAudioPlayerFiles(const QStringList &paths)
{
	for (const QString &path : paths) {
		const QFileInfo info(path);
		if (!info.isFile() || !audioExtensions.contains(info.suffix().toLower()))
			continue;
		const QString absolutePath = info.absoluteFilePath();
		if (audioPlaylistPaths.contains(absolutePath, Qt::CaseInsensitive))
			continue;
		audioPlaylistPaths.push_back(absolutePath);
		auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_MediaVolume), info.fileName(),
						 audioPlaylistList);
		item->setToolTip(absolutePath);
	}
	if (!restoring)
		SaveSettings();
}

void PresenterPanel::PlayAudioPlayerRow(int row)
{
	if (row < 0 || row >= audioPlaylistPaths.size())
		return;
	StopAudioPlayer();
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_string(settings, "local_file", audioPlaylistPaths[row].toUtf8().constData());
	obs_data_set_bool(settings, "audio_only", true);
	obs_data_set_bool(settings, "looping", false);
	obs_data_set_bool(settings, "restart_on_activate", false);
	audioPlayerSource = obs_source_create_private("ffmpeg_source", "OPBS Independent Audio Player", settings);
	if (!audioPlayerSource)
		return;
	obs_source_set_audio_mixers(audioPlayerSource, 0u);
	obs_source_set_monitoring_type(audioPlayerSource, OBS_MONITORING_TYPE_MONITOR_ONLY);
	obs_source_inc_showing(audioPlayerSource);
	obs_source_inc_active(audioPlayerSource);
	obs_source_media_restart(audioPlayerSource);
	audioPlaylistList->setCurrentRow(row);
	audioPlayerTimeline->setEnabled(true);
	audioPlayerPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
}

void PresenterPanel::ToggleAudioPlayer()
{
	if (!audioPlayerSource) {
		PlayAudioPlayerRow(audioPlaylistList ? std::max(audioPlaylistList->currentRow(), 0) : 0);
		return;
	}
	const obs_media_state state = obs_source_media_get_state(audioPlayerSource);
	if (state == OBS_MEDIA_STATE_PLAYING) {
		obs_source_media_play_pause(audioPlayerSource, true);
		audioPlayerPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	} else {
		obs_source_media_play_pause(audioPlayerSource, false);
		audioPlayerPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
	}
}

void PresenterPanel::StopAudioPlayer()
{
	if (audioPlayerSource) {
		obs_source_media_stop(audioPlayerSource);
		obs_source_dec_active(audioPlayerSource);
		obs_source_dec_showing(audioPlayerSource);
		audioPlayerSource = nullptr;
	}
	if (audioPlayerTimeline) {
		audioPlayerTimeline->setValue(0);
		audioPlayerTimeline->setEnabled(false);
	}
	if (audioPlayerPlayPauseButton)
		audioPlayerPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void PresenterPanel::RefreshAudioPlayerTimeline()
{
	if (!audioPlayerSource || audioPlayerTimelineDragging)
		return;
	const int64_t duration = obs_source_media_get_duration(audioPlayerSource);
	const int64_t time = obs_source_media_get_time(audioPlayerSource);
	if (duration > 0)
		audioPlayerTimeline->setValue(int(std::clamp<int64_t>(time * 1000 / duration, 0, 1000)));
	const obs_media_state state = obs_source_media_get_state(audioPlayerSource);
	if (state == OBS_MEDIA_STATE_ENDED)
		StopAudioPlayer();
}

void PresenterPanel::RefreshRecentPresentations()
{
	if (!recentPresentationsList)
		return;
	recentPresentationsList->clear();
	for (int index = 0; index < recentPresentationIds.size(); ++index) {
		const QString name = index < recentPresentationNames.size() ? recentPresentationNames[index]
								     : tr("Presentación %1").arg(index + 1);
		auto *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_FileDialogDetailedView), name,
						 recentPresentationsList);
		item->setData(Qt::UserRole, recentPresentationIds[index]);
	}
}

void PresenterPanel::ActivateRecentPresentation(int row)
{
	if (row < 0 || row >= recentPresentationIds.size())
		return;
	const QString id = recentPresentationIds[row];
	const QString directoryPath = QDir(PresentationsDirectoryPath()).filePath(id);
	if (!QDir(directoryPath).exists())
		return;
	ClearPresentationEntries();
	currentPresentationId = id;
	QFileInfoList slides = QDir(directoryPath).entryInfoList({"*.png"}, QDir::Files, QDir::NoSort);
	std::sort(slides.begin(), slides.end(), [](const QFileInfo &left, const QFileInfo &right) {
		bool leftIsNumber = false;
		bool rightIsNumber = false;
		const int leftNumber = left.completeBaseName().toInt(&leftIsNumber);
		const int rightNumber = right.completeBaseName().toInt(&rightIsNumber);
		if (leftIsNumber && rightIsNumber)
			return leftNumber < rightNumber;
		return left.fileName().localeAwareCompare(right.fileName()) < 0;
	});
	for (const QFileInfo &slide : slides)
		AddMediaFile(slide.absoluteFilePath(), QString::fromLatin1(kPresentationsFolderId), false);
	if (presentationFolderList)
		presentationFolderList->setCurrentRow(1);
	ApplyLibraryFilter();
	SaveSettings();
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
	transmissionAction = main->menuBar()->addAction(tr("Transmisión"));
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
	connect(transmissionAction, &QAction::triggered, this, &PresenterPanel::ShowTransmissionDialog);
	connect(opbsUpdateAction, &QAction::triggered, this, &PresenterPanel::LaunchOpbsUpdater);
	connect(importPowerPoint, &QAction::triggered, this, [this]() { ImportPresentation(false); });
	connect(importPdf, &QAction::triggered, this, [this]() { ImportPresentation(true); });
	for (QAction *action : main->menuBar()->actions())
		action->setVisible(action == fileMenuAction || action == editMenuAction || action == screensAction ||
				   action == soundAction || action == bibleAction || action == transmissionAction ||
				   action == helpMenuAction);

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
	OBSSceneAutoRelease combinedScene = obs_scene_create_private("OPBS Transmission Both");
	OBSSceneAutoRelease cameraScene = obs_scene_create_private("OPBS Transmission Camera");
	OBSSceneAutoRelease presenterScene = obs_scene_create_private("OPBS Transmission Presenter");
	transmissionScene = combinedScene.Get();
	transmissionCameraScene = cameraScene.Get();
	transmissionPresenterScene = presenterScene.Get();
	if (!transmissionScene || !transmissionCameraScene || !transmissionPresenterScene) {
		QMessageBox::critical(this, tr("Transmisión"), tr("No fue posible crear el lienzo de transmisión."));
		stageScene = nullptr;
		transmissionScene = nullptr;
		transmissionCameraScene = nullptr;
		transmissionPresenterScene = nullptr;
		return;
	}
	transmissionPresenterItem = obs_scene_add(transmissionScene, obs_scene_get_source(stageScene));
	transmissionPresenterOnlyItem = obs_scene_add(transmissionPresenterScene, obs_scene_get_source(stageScene));
	obs_sceneitem_set_bounds_alignment(transmissionPresenterItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds_alignment(transmissionPresenterOnlyItem, OBS_ALIGN_CENTER);
	transmissionTransition = obs_source_create_private("move_transition", "OPBS Move Transition", nullptr);
	if (!transmissionTransition) {
		blog(LOG_WARNING, "OPBS Move Transition is unavailable; using fade transition fallback");
		transmissionTransition = obs_source_create_private("fade_transition", "OPBS Transmission Transition", nullptr);
	}
	if (!transmissionTransition) {
		QMessageBox::critical(this, tr("Transmisión"), tr("No fue posible crear la transición de transmisión."));
		stageScene = nullptr;
		transmissionScene = nullptr;
		transmissionCameraScene = nullptr;
		transmissionPresenterScene = nullptr;
		return;
	}
	obs_transition_set_size(transmissionTransition, 1920, 1080);
	obs_transition_set_alignment(transmissionTransition, OBS_ALIGN_CENTER);
	obs_transition_set_scale_type(transmissionTransition, OBS_TRANSITION_SCALE_ASPECT);
	obs_transition_set(transmissionTransition, obs_scene_get_source(transmissionPresenterScene));
	RegisterTransmissionAudioBridge();
	transmissionPresenterAudioSource =
		obs_source_create_private(kTransmissionAudioBridgeId, "OPBS Presenter Audio (Transmission)", nullptr);
	if (!transmissionPresenterAudioSource)
		blog(LOG_ERROR, "OPBS could not create the presenter transmission audio bridge");
	UpdateTransmissionItemBounds();

	BuildTopMenu();
	originalCentralWidget = main->takeCentralWidget();
	if (originalCentralWidget) {
		originalCentralWidget->hide();
		originalCentralWidget->setParent(main);
	}
	main->setCentralWidget(this);
	main->setWindowTitle(tr("OPBS %1 — Presentador integrado").arg(QString::fromLatin1(OPBS_VERSION)));
	main->statusBar()->hide();
	// Esta variante no utiliza el asistente de transmisión/grabación de OBS.
	config_set_bool(App()->GetUserConfig(), "General", "FirstRun", true);
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
	QTimer::singleShot(0, this, [this]() {
		if (dockWorkspace)
			dockWorkspace->show();
		for (QDockWidget *dock : main->findChildren<QDockWidget *>()) {
			if (dock->objectName().startsWith(QStringLiteral("opbs")))
				dock->show();
			else
				dock->hide();
		}
	});
	connect(preview, &OBSQTDisplay::DisplayCreated, this, [this](OBSQTDisplay *display) {
		obs_display_add_draw_callback(display->GetDisplay(), PresenterPanel::RenderPreview, this);
	});
	connect(transmissionPreview, &OBSQTDisplay::DisplayCreated, this, [this](OBSQTDisplay *display) {
		obs_display_add_draw_callback(display->GetDisplay(), PresenterPanel::RenderTransmissionPreview, this);
	});
	connect(main, &OBSBasic::StreamingStarted, this, [this]() {
		StartSecondaryStream();
		UpdateTransmissionButtons();
	});
	connect(main, &OBSBasic::StreamingStopped, this, [this]() {
		StopSecondaryStream();
		UpdateTransmissionButtons();
	});
	connect(main, &OBSBasic::RecordingStarted, this, [this]() { UpdateTransmissionButtons(); });
	connect(main, &OBSBasic::RecordingStopped, this, [this]() { UpdateTransmissionButtons(); });
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
	ApplyTransmissionAudioMix();
	if (transmissionPresenterAudioSource)
		obs_set_output_source(1, transmissionPresenterAudioSource);
	RefreshTransmissionAudioInput();
	ApplyCombinedBackground();
	RefreshCameraSource();
	ApplyTransmissionView(transmissionView, false);
	ApplyPrimaryStreamService();
	UpdateTransmissionButtons();
	QTimer::singleShot(0, this, [this]() {
		if (transmissionTransition)
			obs_set_output_source(0, transmissionTransition);
	});
	timelineTimer->start();
}

void PresenterPanel::Shutdown()
{
	if (!initialized && !stageScene)
		return;
	SaveSettings();
	if (timelineTimer)
		timelineTimer->stop();
	if (audioPlayerTimer)
		audioPlayerTimer->stop();
	StopAudioPlayer();
	if (stageProjector) {
		main->DeleteProjector(stageProjector);
		stageProjector = nullptr;
	}
	if (preview && preview->GetDisplay())
		obs_display_remove_draw_callback(preview->GetDisplay(), PresenterPanel::RenderPreview, this);
	if (transmissionPreview && transmissionPreview->GetDisplay())
		obs_display_remove_draw_callback(transmissionPreview->GetDisplay(), PresenterPanel::RenderTransmissionPreview,
						 this);
	StopSecondaryStream();
	for (uint32_t channel = 0; channel < 6; ++channel)
		obs_set_output_source(channel, nullptr);
	if (audioMeter) {
		obs_volmeter_detach_source(audioMeter);
		obs_volmeter_remove_callback(audioMeter, PresenterPanel::AudioMeterUpdated, this);
		obs_volmeter_destroy(audioMeter);
		audioMeter = nullptr;
	}
	DetachAudioFilters();
	DetachTransmissionPresenterAudio();
	ClearActiveMedia();
	ClearBiblePresentation();
	if (transmissionTransition) {
		obs_transition_force_stop(transmissionTransition);
		obs_transition_clear(transmissionTransition);
	}
	transmissionTransition = nullptr;
	transmissionBackgroundSource = nullptr;
	transmissionInputSource = nullptr;
	transmissionPresenterAudioSource = nullptr;
	cameraSource = nullptr;
	transmissionBackgroundItem = nullptr;
	transmissionCameraOnlyItem = nullptr;
	transmissionPresenterOnlyItem = nullptr;
	transmissionCameraItem = nullptr;
	transmissionPresenterItem = nullptr;
	transmissionScene = nullptr;
	transmissionCameraScene = nullptr;
	transmissionPresenterScene = nullptr;
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
	ReplacePresentationSlides(temporaryPath, result.slidePaths.size(), QFileInfo(path).completeBaseName());
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

void PresenterPanel::ReplacePresentationSlides(const QString &temporaryDirectory, int slideCount,
						const QString &displayName)
{
	const QString basePath = PresentationsDirectoryPath();
	QDir base(basePath);
	const QString temporaryName = QFileInfo(temporaryDirectory).fileName();
	const QString presentationId =
		QStringLiteral("presentation-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
	ClearPresentationEntries();
	if (!base.rename(temporaryName, presentationId)) {
		QMessageBox::critical(main, tr("Importar presentación"),
				      tr("No se pudo guardar la nueva presentación."));
		return;
	}
	currentPresentationId = presentationId;
	recentPresentationIds.removeAll(presentationId);
	recentPresentationIds.prepend(presentationId);
	recentPresentationNames.prepend(displayName.isEmpty() ? tr("Presentación") : displayName);
	while (recentPresentationIds.size() > 4) {
		const QString removedId = recentPresentationIds.takeLast();
		if (recentPresentationNames.size() > recentPresentationIds.size())
			recentPresentationNames.removeLast();
		QDir(base.filePath(removedId)).removeRecursively();
	}
	const QString currentPath = base.filePath(presentationId);
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
	RefreshRecentPresentations();
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
	DetachTransmissionPresenterAudio();
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
	/* La fuente original alimenta exclusivamente el monitor local. La mezcla
	 * de transmisión recibe una copia cruda mediante el puente independiente. */
	obs_source_set_monitoring_type(activeSource, OBS_MONITORING_TYPE_MONITOR_ONLY);
	AttachTransmissionPresenterAudio();
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

void PresenterPanel::PresenterAudioCaptured(void *data, obs_source_t *, const struct audio_data *audio, bool muted)
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel || !audio || muted || !panel->transmissionPresenterAudioSource)
		return;
	obs_audio_info audioInfo = {};
	if (!obs_get_audio_info(&audioInfo))
		return;
	obs_source_audio output = {};
	for (size_t plane = 0; plane < MAX_AV_PLANES; ++plane)
		output.data[plane] = audio->data[plane];
	output.frames = audio->frames;
	output.speakers = audioInfo.speakers;
	output.format = AUDIO_FORMAT_FLOAT_PLANAR;
	output.samples_per_sec = audioInfo.samples_per_sec;
	output.timestamp = audio->timestamp;
	obs_source_output_audio(panel->transmissionPresenterAudioSource, &output);
}

void PresenterPanel::AttachTransmissionPresenterAudio()
{
	if (!activeSource || !transmissionPresenterAudioSource || transmissionPresenterAudioAttached)
		return;
	obs_source_add_audio_capture_callback(activeSource, PresenterPanel::PresenterAudioCaptured, this);
	transmissionPresenterAudioAttached = true;
}

void PresenterPanel::DetachTransmissionPresenterAudio()
{
	if (activeSource && transmissionPresenterAudioAttached)
		obs_source_remove_audio_capture_callback(activeSource, PresenterPanel::PresenterAudioCaptured, this);
	transmissionPresenterAudioAttached = false;
}

OBSSource PresenterPanel::CreateTransmissionAudioInput(const QString &deviceId, const QString &sourceName) const
{
	if (deviceId.isEmpty())
		return nullptr;
	const char *sourceType = obs_get_latest_input_type_id("wasapi_input_capture");
	if (!sourceType)
		return nullptr;
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "device_id", deviceId.toUtf8().constData());
	obs_data_set_bool(settings, "use_device_timing", false);
	OBSSourceAutoRelease source =
		obs_source_create_private(sourceType, sourceName.toUtf8().constData(), settings);
	return source.Get();
}

void PresenterPanel::ApplyTransmissionAudioMix()
{
	if (transmissionPresenterAudioSource) {
		obs_source_set_audio_mixers(transmissionPresenterAudioSource, 1u);
		obs_source_set_monitoring_type(transmissionPresenterAudioSource, OBS_MONITORING_TYPE_NONE);
		obs_source_set_volume(transmissionPresenterAudioSource,
				      obs_db_to_mul(static_cast<float>(transmissionPresenterDb)));
		obs_source_set_muted(transmissionPresenterAudioSource, transmissionPresenterMuted);
	}
	if (transmissionInputSource) {
		obs_source_set_audio_mixers(transmissionInputSource, 1u);
		obs_source_set_monitoring_type(transmissionInputSource, OBS_MONITORING_TYPE_NONE);
		obs_source_set_volume(transmissionInputSource, obs_db_to_mul(static_cast<float>(transmissionInputDb)));
		obs_source_set_muted(transmissionInputSource, transmissionInputMuted);
	}
}

void PresenterPanel::RefreshTransmissionAudioInput()
{
	obs_set_output_source(2, nullptr);
	transmissionInputSource = nullptr;
	if (selectedTransmissionInputId.isEmpty())
		return;
	transmissionInputSource = CreateTransmissionAudioInput(
		selectedTransmissionInputId,
		selectedTransmissionInputName.isEmpty()
			? tr("OPBS Audio - Entrada adicional")
			: tr("OPBS Audio - %1").arg(selectedTransmissionInputName));
	if (!transmissionInputSource) {
		blog(LOG_WARNING, "OPBS could not create transmission audio input '%s'",
		     selectedTransmissionInputId.toUtf8().constData());
		return;
	}
	ApplyTransmissionAudioMix();
	obs_set_output_source(2, transmissionInputSource);
	blog(LOG_INFO, "OPBS transmission audio input ready: '%s' on global channel 2",
	     selectedTransmissionInputName.toUtf8().constData());
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

void PresenterPanel::ShowTransmissionDialog()
{
	QDialog dialog(main);
	dialog.setWindowTitle(tr("Configuración de transmisión"));
	dialog.resize(900, 650);
	auto *root = new QVBoxLayout(&dialog);
	auto *body = new QHBoxLayout();
	auto *sections = new QListWidget(&dialog);
	sections->setFixedWidth(170);
	sections->addItem(tr("📡  Emisión"));
	sections->addItem(tr("🖥  Salida"));
	sections->addItem(tr("📷  Cámaras"));
	sections->addItem(tr("🎙  Audio"));
	sections->addItem(tr("▣  Lienzo de ambos"));
	auto *pages = new QStackedWidget(&dialog);
	body->addWidget(sections);
	body->addWidget(pages, 1);
	root->addLayout(body, 1);

	struct DestinationControls {
		QCheckBox *enabled = nullptr;
		QComboBox *service = nullptr;
		QLineEdit *server = nullptr;
		QLineEdit *key = nullptr;
	};
	DestinationControls destinationControls[2];
	auto defaultServer = [](const QString &service) {
		if (service == QStringLiteral("Facebook"))
			return QStringLiteral("rtmps://rtmp-api.facebook.com:443/rtmp/");
		if (service == QStringLiteral("YouTube"))
			return QStringLiteral("rtmps://a.rtmps.youtube.com:443/live2");
		return QString();
	};

	auto *emissionPage = new QWidget(pages);
	auto *emissionLayout = new QVBoxLayout(emissionPage);
	auto *emissionIntro = new QLabel(
		tr("OPBS puede enviar la misma composición a dos destinos de forma nativa. Configura al menos el primero."),
		emissionPage);
	emissionIntro->setWordWrap(true);
	emissionLayout->addWidget(emissionIntro);
	for (int index = 0; index < 2; ++index) {
		auto *group = new QGroupBox(tr("Destino %1").arg(index + 1), emissionPage);
		auto *form = new QFormLayout(group);
		if (index == 1) {
			destinationControls[index].enabled = new QCheckBox(tr("Activar segundo destino"), group);
			destinationControls[index].enabled->setChecked(streamDestinations[index].enabled);
			form->addRow(destinationControls[index].enabled);
		}
		destinationControls[index].service = new QComboBox(group);
		destinationControls[index].service->addItems({QStringLiteral("YouTube"), QStringLiteral("Facebook"),
							      QStringLiteral("Personalizado")});
		destinationControls[index].service->setCurrentText(streamDestinations[index].service);
		destinationControls[index].server = new QLineEdit(streamDestinations[index].server, group);
		if (destinationControls[index].server->text().isEmpty())
			destinationControls[index].server->setText(defaultServer(streamDestinations[index].service));
		destinationControls[index].key = new QLineEdit(streamDestinations[index].key, group);
		destinationControls[index].key->setEchoMode(QLineEdit::Password);
		destinationControls[index].key->setPlaceholderText(tr("Clave de transmisión"));
		form->addRow(tr("Servicio"), destinationControls[index].service);
		form->addRow(tr("Servidor"), destinationControls[index].server);
		form->addRow(tr("Clave"), destinationControls[index].key);
		connect(destinationControls[index].service, &QComboBox::currentTextChanged, group,
			[destinationControls, index, defaultServer](const QString &service) {
				if (service != QStringLiteral("Personalizado"))
					destinationControls[index].server->setText(defaultServer(service));
			});
		emissionLayout->addWidget(group);
	}
	emissionLayout->addStretch();
	pages->addWidget(emissionPage);

	auto *outputPage = new QWidget(pages);
	auto *outputLayout = new QVBoxLayout(outputPage);
	auto *fixedVideo = new QGroupBox(tr("Video interno"), outputPage);
	auto *fixedVideoLayout = new QFormLayout(fixedVideo);
	fixedVideoLayout->addRow(tr("Resolución del lienzo"), new QLabel(tr("1920 × 1080 (16:9)"), fixedVideo));
	fixedVideoLayout->addRow(tr("Resolución de salida"), new QLabel(tr("1920 × 1080"), fixedVideo));
	fixedVideoLayout->addRow(tr("Fotogramas por segundo"), new QLabel(tr("60 FPS"), fixedVideo));
	outputLayout->addWidget(fixedVideo);
	auto *outputGroup = new QGroupBox(tr("Salida"), outputPage);
	auto *outputForm = new QFormLayout(outputGroup);
	auto *videoBitrate = new QSpinBox(outputGroup);
	videoBitrate->setRange(1000, 51000);
	videoBitrate->setSuffix(tr(" kbps"));
	videoBitrate->setValue(streamVideoBitrate);
	auto *audioBitrate = new QSpinBox(outputGroup);
	audioBitrate->setRange(64, 320);
	audioBitrate->setSingleStep(32);
	audioBitrate->setSuffix(tr(" kbps"));
	audioBitrate->setValue(streamAudioBitrate);
	auto *recordingPathEdit = new QLineEdit(recordingPath, outputGroup);
	auto *recordingPathRow = new QHBoxLayout();
	auto *browseRecordingPath = new QPushButton(tr("Examinar…"), outputGroup);
	recordingPathRow->addWidget(recordingPathEdit, 1);
	recordingPathRow->addWidget(browseRecordingPath);
	outputForm->addRow(tr("Tasa de bits de video"), videoBitrate);
	outputForm->addRow(tr("Tasa de bits de audio"), audioBitrate);
	outputForm->addRow(tr("Carpeta de grabaciones"), recordingPathRow);
	connect(browseRecordingPath, &QPushButton::clicked, &dialog, [&dialog, recordingPathEdit]() {
		const QString path = QFileDialog::getExistingDirectory(&dialog, QObject::tr("Carpeta de grabaciones"),
								      recordingPathEdit->text());
		if (!path.isEmpty())
			recordingPathEdit->setText(path);
	});
	outputLayout->addWidget(outputGroup);
	outputLayout->addStretch();
	pages->addWidget(outputPage);

	auto *cameraPage = new QWidget(pages);
	auto *cameraLayout = new QVBoxLayout(cameraPage);
	auto *cameraGroup = new QGroupBox(tr("Configurar cámara"), cameraPage);
	auto *cameraForm = new QFormLayout(cameraGroup);
	const QString originalCameraId = selectedCameraId;
	const QString originalCameraName = selectedCameraName;
	const bool originalCameraEnabled = cameraEnabled;
	auto *cameraSelector = new QComboBox(cameraGroup);
	cameraSelector->addItem(tr("Sin cámara"), QString());
	OBSProperties cameraProperties = obs_get_source_properties("dshow_input");
	if (cameraProperties) {
		obs_property_t *devices = obs_properties_get(cameraProperties, "video_device_id");
		if (devices) {
			const size_t count = obs_property_list_item_count(devices);
			for (size_t index = 0; index < count; ++index) {
				const QString name = QString::fromUtf8(obs_property_list_item_name(devices, index));
				const QString id = QString::fromUtf8(obs_property_list_item_string(devices, index));
				if (!id.isEmpty())
					cameraSelector->addItem(name, id);
			}
		}
	}
	int cameraIndex = cameraSelector->findData(selectedCameraId);
	if (cameraIndex < 0 && !selectedCameraId.isEmpty()) {
		cameraSelector->addItem(selectedCameraName.isEmpty() ? tr("Cámara no disponible") : selectedCameraName,
					selectedCameraId);
		cameraIndex = cameraSelector->count() - 1;
	}
	cameraSelector->setCurrentIndex(std::max(cameraIndex, 0));
	cameraForm->addRow(tr("Asignar cámara"), cameraSelector);
	auto *cameraToggle = new QPushButton(cameraGroup);
	auto updateCameraToggle = [this, cameraSelector, cameraToggle]() {
		const bool hasCamera = !cameraSelector->currentData().toString().isEmpty();
		cameraToggle->setEnabled(hasCamera);
		cameraToggle->setText(hasCamera && cameraEnabled ? tr("Desactivar") : tr("Activar"));
	};
	cameraForm->addRow(QString(), cameraToggle);
	connect(cameraSelector, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
		[this, cameraSelector, updateCameraToggle](int) {
			const QString newId = cameraSelector->currentData().toString();
			const bool changed = newId != selectedCameraId;
			selectedCameraId = newId;
			selectedCameraName = newId.isEmpty() ? QString() : cameraSelector->currentText();
			if (changed)
				cameraEnabled = !newId.isEmpty();
			RefreshCameraSource();
			updateCameraToggle();
		});
	connect(cameraToggle, &QPushButton::clicked, &dialog, [this, cameraSelector, updateCameraToggle]() {
		if (cameraSelector->currentData().toString().isEmpty())
			return;
		SetCameraEnabled(!cameraEnabled);
		updateCameraToggle();
	});
	updateCameraToggle();
	cameraLayout->addWidget(cameraGroup);
	auto *cameraHint = new QLabel(
		tr("La cámara seleccionada se usará en los modos Cámaras y Ambos. Si vuelves a conectar "
		   "una capturadora, pulsa Desactivar y luego Activar para reiniciarla."),
		cameraPage);
	cameraHint->setWordWrap(true);
	cameraLayout->addWidget(cameraHint);
	cameraLayout->addStretch();
	pages->addWidget(cameraPage);

	auto *audioPage = new QWidget(pages);
	auto *audioLayout = new QVBoxLayout(audioPage);
	auto *audioSourcesGroup = new QGroupBox(tr("Entradas de audio de transmisión"), audioPage);
	auto *audioSourcesForm = new QFormLayout(audioSourcesGroup);
	auto *presenterAudioFixed = new QLineEdit(tr("Audio del presentador (fijo)"), audioSourcesGroup);
	presenterAudioFixed->setReadOnly(true);
	presenterAudioFixed->setEnabled(false);
	auto *transmissionInputSelector = new QComboBox(audioSourcesGroup);
	transmissionInputSelector->addItem(tr("Sin entrada adicional"), QString());
	OBSProperties inputProperties = obs_get_source_properties("wasapi_input_capture");
	if (inputProperties) {
		obs_property_t *devices = obs_properties_get(inputProperties, "device_id");
		if (devices) {
			const size_t count = obs_property_list_item_count(devices);
			for (size_t index = 0; index < count; ++index) {
				const QString name = QString::fromUtf8(obs_property_list_item_name(devices, index));
				const QString id = QString::fromUtf8(obs_property_list_item_string(devices, index));
				if (!id.isEmpty())
					transmissionInputSelector->addItem(name, id);
			}
		}
	}
	int transmissionInputIndex = transmissionInputSelector->findData(selectedTransmissionInputId);
	if (transmissionInputIndex < 0 && !selectedTransmissionInputId.isEmpty()) {
		transmissionInputSelector->addItem(
			selectedTransmissionInputName.isEmpty() ? tr("Entrada no disponible")
								: selectedTransmissionInputName,
			selectedTransmissionInputId);
		transmissionInputIndex = transmissionInputSelector->count() - 1;
	}
	transmissionInputSelector->setCurrentIndex(std::max(transmissionInputIndex, 0));
	audioSourcesForm->addRow(tr("Presentador"), presenterAudioFixed);
	audioSourcesForm->addRow(tr("Entrada adicional"), transmissionInputSelector);
	audioLayout->addWidget(audioSourcesGroup);

	auto *transmissionMixerGroup = new QGroupBox(tr("Mezclador de audio de transmisión"), audioPage);
	auto *transmissionMixerLayout = new QVBoxLayout(transmissionMixerGroup);
	const float originalPresenterVolume =
		transmissionPresenterAudioSource ? obs_source_get_volume(transmissionPresenterAudioSource) : 1.0f;
	const bool originalPresenterMuted =
		transmissionPresenterAudioSource && obs_source_muted(transmissionPresenterAudioSource);
	QPointer<VolumeControl> presenterTransmissionMixer;
	if (transmissionPresenterAudioSource) {
		presenterTransmissionMixer =
			new VolumeControl(transmissionPresenterAudioSource, transmissionMixerGroup, false);
		presenterTransmissionMixer->setGlobalInMixer(true);
		transmissionMixerLayout->addWidget(presenterTransmissionMixer);
	} else {
		transmissionMixerLayout->addWidget(
			new QLabel(tr("La entrada del presentador no está disponible."), transmissionMixerGroup));
	}
	auto *inputMixerContainer = new QWidget(transmissionMixerGroup);
	auto *inputMixerLayout = new QVBoxLayout(inputMixerContainer);
	inputMixerLayout->setContentsMargins(0, 0, 0, 0);
	transmissionMixerLayout->addWidget(inputMixerContainer);
	auto *audioHint = new QLabel(
		tr("Estos controles solo afectan la transmisión y la grabación; no cambian el volumen que sale por "
		   "los altavoces del presentador."),
		transmissionMixerGroup);
	audioHint->setWordWrap(true);
	transmissionMixerLayout->addWidget(audioHint);
	audioLayout->addWidget(transmissionMixerGroup);
	audioLayout->addStretch();
	pages->addWidget(audioPage);

	OBSSource pendingTransmissionInput;
	QPointer<VolumeControl> pendingInputMixer;
	bool pendingInputActive = false;
	double pendingInputDb = transmissionInputDb;
	bool pendingInputMuted = transmissionInputMuted;
	auto releasePendingInput = [&]() {
		if (pendingTransmissionInput) {
			const float db = obs_mul_to_db(obs_source_get_volume(pendingTransmissionInput));
			if (std::isfinite(db))
				pendingInputDb = std::clamp<double>(db, -60.0, 20.0);
			pendingInputMuted = obs_source_muted(pendingTransmissionInput);
		}
		while (QLayoutItem *item = inputMixerLayout->takeAt(0)) {
			delete item->widget();
			delete item;
		}
		pendingInputMixer = nullptr;
		if (pendingTransmissionInput && pendingInputActive) {
			obs_source_dec_active(pendingTransmissionInput);
			obs_source_dec_showing(pendingTransmissionInput);
		}
		pendingInputActive = false;
		pendingTransmissionInput = nullptr;
	};
	auto rebuildPendingInput = [&]() {
		releasePendingInput();
		const QString deviceId = transmissionInputSelector->currentData().toString();
		if (deviceId.isEmpty()) {
			inputMixerLayout->addWidget(
				new QLabel(tr("No se seleccionó una entrada adicional."), inputMixerContainer));
			return;
		}
		pendingTransmissionInput = CreateTransmissionAudioInput(deviceId, tr("Entrada adicional (prueba)"));
		if (!pendingTransmissionInput) {
			inputMixerLayout->addWidget(
				new QLabel(tr("No fue posible abrir esta entrada de audio."), inputMixerContainer));
			return;
		}
		obs_source_set_audio_mixers(pendingTransmissionInput, 1u);
		obs_source_set_monitoring_type(pendingTransmissionInput, OBS_MONITORING_TYPE_NONE);
		obs_source_set_volume(pendingTransmissionInput, obs_db_to_mul(static_cast<float>(pendingInputDb)));
		obs_source_set_muted(pendingTransmissionInput, pendingInputMuted);
		obs_source_inc_showing(pendingTransmissionInput);
		obs_source_inc_active(pendingTransmissionInput);
		pendingInputActive = true;
		pendingInputMixer = new VolumeControl(pendingTransmissionInput, inputMixerContainer, false);
		pendingInputMixer->setGlobalInMixer(true);
		inputMixerLayout->addWidget(pendingInputMixer);
	};
	connect(transmissionInputSelector, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
		[&](int) { rebuildPendingInput(); });
	rebuildPendingInput();

	auto *bothScroll = new QScrollArea(pages);
	bothScroll->setWidgetResizable(true);
	auto *bothPage = new QWidget(bothScroll);
	auto *bothLayout = new QVBoxLayout(bothPage);
	auto *canvasPreview = new QFrame(bothPage);
	canvasPreview->setFixedSize(480, 270);
	canvasPreview->setFrameShape(QFrame::StyledPanel);
	auto *cameraPreview = new QLabel(tr("Cámara"), canvasPreview);
	cameraPreview->setAlignment(Qt::AlignCenter);
	cameraPreview->setStyleSheet("background:#d6aa00; color:#111; border:1px solid #ffe16b; font-weight:700;");
	auto *presenterPreview = new QLabel(tr("Presentador"), canvasPreview);
	presenterPreview->setAlignment(Qt::AlignCenter);
	presenterPreview->setStyleSheet("background:#08090c; color:white; border:1px solid #3a3f4b; font-weight:700;");
	auto *previewRow = new QHBoxLayout();
	previewRow->addStretch();
	previewRow->addWidget(canvasPreview);
	previewRow->addStretch();
	bothLayout->addLayout(previewRow);

	auto makePercentSpin = [bothPage](double value) {
		auto *spin = new QDoubleSpinBox(bothPage);
		spin->setRange(0.0, 100.0);
		spin->setDecimals(1);
		spin->setSingleStep(0.5);
		spin->setSuffix(" %");
		spin->setValue(value);
		return spin;
	};
	auto *compositionGroup = new QGroupBox(tr("Composición"), bothPage);
	auto *compositionGrid = new QGridLayout(compositionGroup);
	compositionGrid->addWidget(new QLabel(tr("Elemento"), compositionGroup), 0, 0);
	compositionGrid->addWidget(new QLabel(tr("X"), compositionGroup), 0, 1);
	compositionGrid->addWidget(new QLabel(tr("Y"), compositionGroup), 0, 2);
	compositionGrid->addWidget(new QLabel(tr("Ancho"), compositionGroup), 0, 3);
	compositionGrid->addWidget(new QLabel(tr("Alto"), compositionGroup), 0, 4);
	auto *cameraX = makePercentSpin(combinedCameraX);
	auto *cameraY = makePercentSpin(combinedCameraY);
	auto *cameraWidth = makePercentSpin(combinedCameraWidth);
	auto *cameraHeight = makePercentSpin(combinedCameraHeight);
	auto *presenterX = makePercentSpin(combinedPresenterX);
	auto *presenterY = makePercentSpin(combinedPresenterY);
	auto *presenterWidth = makePercentSpin(combinedPresenterWidth);
	auto *presenterHeight = makePercentSpin(combinedPresenterHeight);
	compositionGrid->addWidget(new QLabel(tr("Cámara"), compositionGroup), 1, 0);
	compositionGrid->addWidget(cameraX, 1, 1);
	compositionGrid->addWidget(cameraY, 1, 2);
	compositionGrid->addWidget(cameraWidth, 1, 3);
	compositionGrid->addWidget(cameraHeight, 1, 4);
	compositionGrid->addWidget(new QLabel(tr("Presentador"), compositionGroup), 2, 0);
	compositionGrid->addWidget(presenterX, 2, 1);
	compositionGrid->addWidget(presenterY, 2, 2);
	compositionGrid->addWidget(presenterWidth, 2, 3);
	compositionGrid->addWidget(presenterHeight, 2, 4);
	bothLayout->addWidget(compositionGroup);

	auto *backgroundGroup = new QGroupBox(tr("Fondo"), bothPage);
	auto *backgroundForm = new QFormLayout(backgroundGroup);
	auto *backgroundType = new QComboBox(backgroundGroup);
	backgroundType->addItem(tr("Color"), "color");
	backgroundType->addItem(tr("Imagen"), "image");
	backgroundType->addItem(tr("Video"), "video");
	backgroundType->setCurrentIndex(std::max(backgroundType->findData(combinedBackgroundType), 0));
	QColor pendingBackgroundColor(combinedBackgroundColor);
	if (!pendingBackgroundColor.isValid())
		pendingBackgroundColor = Qt::black;
	auto *colorButton = new QPushButton(pendingBackgroundColor.name(QColor::HexRgb), backgroundGroup);
	auto *backgroundPathEdit = new QLineEdit(combinedBackgroundPath, backgroundGroup);
	auto *backgroundBrowse = new QPushButton(tr("Examinar…"), backgroundGroup);
	auto *backgroundPathRow = new QHBoxLayout();
	backgroundPathRow->addWidget(backgroundPathEdit, 1);
	backgroundPathRow->addWidget(backgroundBrowse);
	auto *backgroundLoop = new QCheckBox(tr("Repetir video en bucle"), backgroundGroup);
	backgroundLoop->setChecked(combinedBackgroundLoop);
	backgroundForm->addRow(tr("Tipo"), backgroundType);
	backgroundForm->addRow(tr("Color"), colorButton);
	backgroundForm->addRow(tr("Archivo"), backgroundPathRow);
	backgroundForm->addRow(QString(), backgroundLoop);
	bothLayout->addWidget(backgroundGroup);

	auto *transitionGroup = new QGroupBox(tr("Transición"), bothPage);
	auto *transitionForm = new QFormLayout(transitionGroup);
	auto *transitionName = new QLabel(tr("Move Transition (predeterminada)"), transitionGroup);
	auto *transitionDuration = new QSpinBox(transitionGroup);
	transitionDuration->setRange(100, 3000);
	transitionDuration->setSuffix(tr(" ms"));
	transitionDuration->setValue(transmissionTransitionDuration);
	transitionForm->addRow(tr("Tipo"), transitionName);
	transitionForm->addRow(tr("Duración"), transitionDuration);
	bothLayout->addWidget(transitionGroup);
	bothLayout->addStretch();
	bothScroll->setWidget(bothPage);
	pages->addWidget(bothScroll);

	auto refreshCanvasPreview = [&]() {
		const int previewWidth = canvasPreview->width();
		const int previewHeight = canvasPreview->height();
		auto geometryFromControls = [previewWidth, previewHeight](QDoubleSpinBox *x, QDoubleSpinBox *y,
								     QDoubleSpinBox *width, QDoubleSpinBox *height) {
			return QRect(qRound(previewWidth * x->value() / 100.0), qRound(previewHeight * y->value() / 100.0),
				     std::max(qRound(previewWidth * width->value() / 100.0), 1),
				     std::max(qRound(previewHeight * height->value() / 100.0), 1));
		};
		cameraPreview->setGeometry(geometryFromControls(cameraX, cameraY, cameraWidth, cameraHeight));
		presenterPreview->setGeometry(
			geometryFromControls(presenterX, presenterY, presenterWidth, presenterHeight));
		const QString type = backgroundType->currentData().toString();
		if (type == QStringLiteral("color"))
			canvasPreview->setStyleSheet(QString("background:%1; border:1px solid #3a3f4b;")
						     .arg(pendingBackgroundColor.name(QColor::HexRgb)));
		else
			canvasPreview->setStyleSheet("background:#707070; border:1px solid #3a3f4b;");
	};
	for (QDoubleSpinBox *spin : {cameraX, cameraY, cameraWidth, cameraHeight, presenterX, presenterY,
				     presenterWidth, presenterHeight})
		connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), &dialog,
			[&](double) { refreshCanvasPreview(); });
	auto updateBackgroundControls = [&]() {
		const QString type = backgroundType->currentData().toString();
		colorButton->setVisible(type == QStringLiteral("color"));
		backgroundPathEdit->setVisible(type != QStringLiteral("color"));
		backgroundBrowse->setVisible(type != QStringLiteral("color"));
		backgroundLoop->setVisible(type == QStringLiteral("video"));
		refreshCanvasPreview();
	};
	connect(backgroundType, qOverload<int>(&QComboBox::currentIndexChanged), &dialog,
		[&](int) { updateBackgroundControls(); });
	connect(colorButton, &QPushButton::clicked, &dialog, [&]() {
		const QColor selected = QColorDialog::getColor(pendingBackgroundColor, &dialog, tr("Color del fondo"));
		if (!selected.isValid())
			return;
		pendingBackgroundColor = selected;
		colorButton->setText(selected.name(QColor::HexRgb));
		refreshCanvasPreview();
	});
	connect(backgroundBrowse, &QPushButton::clicked, &dialog, [&]() {
		const bool video = backgroundType->currentData().toString() == QStringLiteral("video");
		const QString filter = video ? tr("Videos (*.mp4 *.mkv *.mov *.webm *.avi);;Todos (*.*)")
					     : tr("Imágenes (*.png *.jpg *.jpeg *.webp *.bmp);;Todos (*.*)");
		const QString path = QFileDialog::getOpenFileName(&dialog, tr("Seleccionar fondo"),
							       backgroundPathEdit->text(), filter);
		if (!path.isEmpty())
			backgroundPathEdit->setText(path);
	});
	updateBackgroundControls();

	connect(sections, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
	sections->setCurrentRow(0);
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
	buttons->button(QDialogButtonBox::Save)->setText(tr("Guardar"));
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	root->addWidget(buttons);
	const bool accepted = dialog.exec() == QDialog::Accepted;
	double pendingPresenterDb = transmissionPresenterDb;
	bool pendingPresenterMuted = transmissionPresenterMuted;
	if (transmissionPresenterAudioSource) {
		const float db = obs_mul_to_db(obs_source_get_volume(transmissionPresenterAudioSource));
		if (std::isfinite(db))
			pendingPresenterDb = std::clamp<double>(db, -60.0, 20.0);
		pendingPresenterMuted = obs_source_muted(transmissionPresenterAudioSource);
	}
	const QString pendingInputId = transmissionInputSelector->currentData().toString();
	const QString pendingInputName = transmissionInputSelector->currentText();
	releasePendingInput();
	if (presenterTransmissionMixer)
		delete presenterTransmissionMixer.data();
	if (!accepted) {
		selectedCameraId = originalCameraId;
		selectedCameraName = originalCameraName;
		cameraEnabled = originalCameraEnabled;
		RefreshCameraSource();
		if (transmissionPresenterAudioSource) {
			obs_source_set_volume(transmissionPresenterAudioSource, originalPresenterVolume);
			obs_source_set_muted(transmissionPresenterAudioSource, originalPresenterMuted);
			obs_source_set_monitoring_type(transmissionPresenterAudioSource, OBS_MONITORING_TYPE_NONE);
		}
		return;
	}

	for (int index = 0; index < 2; ++index) {
		streamDestinations[index].service = destinationControls[index].service->currentText();
		streamDestinations[index].server = destinationControls[index].server->text().trimmed();
		streamDestinations[index].key = destinationControls[index].key->text().trimmed();
		streamDestinations[index].enabled = index == 0 || destinationControls[index].enabled->isChecked();
	}
	streamVideoBitrate = videoBitrate->value();
	streamAudioBitrate = audioBitrate->value();
	recordingPath = recordingPathEdit->text().trimmed();
	selectedCameraId = cameraSelector->currentData().toString();
	selectedCameraName = selectedCameraId.isEmpty() ? QString() : cameraSelector->currentText();
	if (selectedCameraId.isEmpty())
		cameraEnabled = false;
	selectedTransmissionInputId = pendingInputId;
	selectedTransmissionInputName = pendingInputId.isEmpty() ? QString() : pendingInputName;
	transmissionPresenterDb = pendingPresenterDb;
	transmissionPresenterMuted = pendingPresenterMuted;
	transmissionInputDb = pendingInputDb;
	transmissionInputMuted = pendingInputMuted;
	combinedCameraX = cameraX->value();
	combinedCameraY = cameraY->value();
	combinedCameraWidth = cameraWidth->value();
	combinedCameraHeight = cameraHeight->value();
	combinedPresenterX = presenterX->value();
	combinedPresenterY = presenterY->value();
	combinedPresenterWidth = presenterWidth->value();
	combinedPresenterHeight = presenterHeight->value();
	combinedBackgroundType = backgroundType->currentData().toString();
	combinedBackgroundColor = pendingBackgroundColor.name(QColor::HexRgb);
	combinedBackgroundPath = backgroundPathEdit->text().trimmed();
	combinedBackgroundLoop = backgroundLoop->isChecked();
	transmissionTransitionDuration = transitionDuration->value();
	ApplyCombinedBackground();
	UpdateTransmissionItemBounds();
	RefreshCameraSource();
	ApplyTransmissionAudioMix();
	RefreshTransmissionAudioInput();
	ApplyPrimaryStreamService();
	if (!main->Active())
		main->ResetOutputs();
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
	const bool placeholderTool = folderId == QString::fromLatin1(kCaptureFolderId) ||
				     folderId == QString::fromLatin1(kNdiFolderId);
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
	if (placeholderTool) {
		mediaList->hide();
		emptyState->setText(folderId == QString::fromLatin1(kCaptureFolderId)
					    ? tr("La herramienta Captura se habilitará en una próxima versión")
					    : tr("La herramienta NDI se habilitará en una próxima versión"));
		emptyState->show();
		mediaCount->setText(tr("Próximamente"));
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
	emptyState->setText(query.isEmpty() ? (folderId == QString::fromLatin1(kPresentationsFolderId)
						 ? tr("Importa o elige una presentación reciente")
						 : tr("Arrastra aquí imágenes, videos o audio para comenzar"))
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
	selectedCameraId = settings.value("transmission/cameraId").toString();
	selectedCameraName = settings.value("transmission/cameraName").toString();
	cameraEnabled = settings.value("transmission/cameraEnabled", true).toBool();
	selectedTransmissionInputId = settings.value("transmission/audio/inputId").toString();
	selectedTransmissionInputName = settings.value("transmission/audio/inputName").toString();
	transmissionPresenterDb =
		std::clamp(settings.value("transmission/audio/presenterDb", 0.0).toDouble(), -60.0, 20.0);
	transmissionInputDb =
		std::clamp(settings.value("transmission/audio/inputDb", 0.0).toDouble(), -60.0, 20.0);
	transmissionPresenterMuted = settings.value("transmission/audio/presenterMuted", false).toBool();
	transmissionInputMuted = settings.value("transmission/audio/inputMuted", false).toBool();
	streamVideoBitrate = std::clamp(settings.value("transmission/videoBitrate", 6000).toInt(), 1000, 51000);
	streamAudioBitrate = std::clamp(settings.value("transmission/audioBitrate", 160).toInt(), 64, 320);
	recordingPath = settings.value("transmission/recordingPath",
				       QString::fromUtf8(config_get_string(main->Config(), "SimpleOutput", "FilePath")))
			.toString();
	streamDestinations[0].service = settings.value("transmission/destination1/service", "YouTube").toString();
	streamDestinations[0].server = settings
					       .value("transmission/destination1/server",
						      "rtmps://a.rtmps.youtube.com:443/live2")
					       .toString();
	streamDestinations[0].key = settings.value("transmission/destination1/key").toString();
	streamDestinations[0].enabled = true;
	streamDestinations[1].service = settings.value("transmission/destination2/service", "Facebook").toString();
	streamDestinations[1].server = settings
					       .value("transmission/destination2/server",
						      "rtmps://rtmp-api.facebook.com:443/rtmp/")
					       .toString();
	streamDestinations[1].key = settings.value("transmission/destination2/key").toString();
	streamDestinations[1].enabled = settings.value("transmission/destination2/enabled", false).toBool();
	transmissionView = static_cast<TransmissionView>(
		std::clamp(settings.value("transmission/view", int(TransmissionView::Presenter)).toInt(), 0, 2));
	transmissionTransitionDuration =
		std::clamp(settings.value("transmission/transitionDuration", 600).toInt(), 100, 3000);
	combinedBackgroundType = settings.value("transmission/both/backgroundType", "color").toString();
	combinedBackgroundColor = settings.value("transmission/both/backgroundColor", "#000000").toString();
	combinedBackgroundPath = settings.value("transmission/both/backgroundPath").toString();
	combinedBackgroundLoop = settings.value("transmission/both/backgroundLoop", true).toBool();
	auto loadPercent = [&settings](const char *key, double fallback) {
		return std::clamp(settings.value(QString::fromLatin1(key), fallback).toDouble(), 0.0, 100.0);
	};
	combinedCameraX = loadPercent("transmission/both/cameraX", 3.5);
	combinedCameraY = loadPercent("transmission/both/cameraY", 31.0);
	combinedCameraWidth = loadPercent("transmission/both/cameraWidth", 35.0);
	combinedCameraHeight = loadPercent("transmission/both/cameraHeight", 35.0);
	combinedPresenterX = loadPercent("transmission/both/presenterX", 41.0);
	combinedPresenterY = loadPercent("transmission/both/presenterY", 19.0);
	combinedPresenterWidth = loadPercent("transmission/both/presenterWidth", 57.0);
	combinedPresenterHeight = loadPercent("transmission/both/presenterHeight", 57.0);
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
	audioPlaylistPaths.clear();
	if (audioPlaylistList)
		audioPlaylistList->clear();
	AddAudioPlayerFiles(settings.value("audioPlayer/files").toStringList());
	recentPresentationIds = settings.value("presentations/recentIds").toStringList();
	recentPresentationNames = settings.value("presentations/recentNames").toStringList();
	currentPresentationId = settings.value("presentations/currentId").toString();
	while (recentPresentationIds.size() > 4)
		recentPresentationIds.removeLast();
	while (recentPresentationNames.size() > 4)
		recentPresentationNames.removeLast();
	RefreshRecentPresentations();
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
	const int layoutVersion = settings.value("window/layoutVersion", 0).toInt();
	const QByteArray dockState = settings.value("window/dockWorkspace").toByteArray();
	if (dockWorkspace && layoutVersion >= 14 && !dockState.isEmpty())
		dockWorkspace->restoreState(dockState, 14);
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

	QStringList missingMediaNames;
	const auto loadMediaPath = [this, &missingMediaNames](const QString &path, const QString &folderId) {
		const QFileInfo file(path);
		if (path.isEmpty())
			return;
		if (!file.exists() || !file.isFile()) {
			missingMediaNames.append(file.fileName().isEmpty() ? path : file.fileName());
			return;
		}
		AddMediaFile(path, folderId, false);
	};
	const int mediaEntryCount = settings.beginReadArray("media");
	for (int index = 0; index < mediaEntryCount; ++index) {
		settings.setArrayIndex(index);
		loadMediaPath(settings.value("path").toString(),
			      settings.value("folderId", "general").toString());
	}
	settings.endArray();
	if (mediaEntryCount == 0) {
		for (const QString &path : settings.value("library/files").toStringList())
			loadMediaPath(path, "general");
	}
	ApplyLibraryFilter();
	ResolveSelectedMonitor();
	SetStageEnabled(stageEnabled, false);
	restoring = false;
	SaveSettings();
	missingMediaNames.removeDuplicates();
	if (!missingMediaNames.isEmpty()) {
		QTimer::singleShot(0, this, [this, missingMediaNames]() {
			QMessageBox notice(main);
			notice.setWindowTitle(tr("Fuentes eliminadas"));
			notice.setIcon(QMessageBox::Information);
			notice.setText(tr("Se eliminaron unas fuentes de la computadora por el usuario."));
			notice.setInformativeText(QStringLiteral("• ") + missingMediaNames.join(QStringLiteral("\n• ")));
			notice.addButton(tr("Aceptar"), QMessageBox::AcceptRole);
			notice.exec();
		});
	}
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
	settings.setValue("audioPlayer/files", audioPlaylistPaths);
	settings.setValue("presentations/recentIds", recentPresentationIds);
	settings.setValue("presentations/recentNames", recentPresentationNames);
	settings.setValue("presentations/currentId", currentPresentationId);
	settings.setValue("transmission/cameraId", selectedCameraId);
	settings.setValue("transmission/cameraName", selectedCameraName);
	settings.setValue("transmission/cameraEnabled", cameraEnabled);
	settings.setValue("transmission/audio/inputId", selectedTransmissionInputId);
	settings.setValue("transmission/audio/inputName", selectedTransmissionInputName);
	settings.setValue("transmission/audio/presenterDb", transmissionPresenterDb);
	settings.setValue("transmission/audio/inputDb", transmissionInputDb);
	settings.setValue("transmission/audio/presenterMuted", transmissionPresenterMuted);
	settings.setValue("transmission/audio/inputMuted", transmissionInputMuted);
	settings.setValue("transmission/videoBitrate", streamVideoBitrate);
	settings.setValue("transmission/audioBitrate", streamAudioBitrate);
	settings.setValue("transmission/recordingPath", recordingPath);
	settings.setValue("transmission/view", int(transmissionView));
	settings.setValue("transmission/transitionDuration", transmissionTransitionDuration);
	settings.setValue("transmission/both/backgroundType", combinedBackgroundType);
	settings.setValue("transmission/both/backgroundColor", combinedBackgroundColor);
	settings.setValue("transmission/both/backgroundPath", combinedBackgroundPath);
	settings.setValue("transmission/both/backgroundLoop", combinedBackgroundLoop);
	settings.setValue("transmission/both/cameraX", combinedCameraX);
	settings.setValue("transmission/both/cameraY", combinedCameraY);
	settings.setValue("transmission/both/cameraWidth", combinedCameraWidth);
	settings.setValue("transmission/both/cameraHeight", combinedCameraHeight);
	settings.setValue("transmission/both/presenterX", combinedPresenterX);
	settings.setValue("transmission/both/presenterY", combinedPresenterY);
	settings.setValue("transmission/both/presenterWidth", combinedPresenterWidth);
	settings.setValue("transmission/both/presenterHeight", combinedPresenterHeight);
	settings.setValue("transmission/destination1/service", streamDestinations[0].service);
	settings.setValue("transmission/destination1/server", streamDestinations[0].server);
	settings.setValue("transmission/destination1/key", streamDestinations[0].key);
	settings.setValue("transmission/destination2/service", streamDestinations[1].service);
	settings.setValue("transmission/destination2/server", streamDestinations[1].server);
	settings.setValue("transmission/destination2/key", streamDestinations[1].key);
	settings.setValue("transmission/destination2/enabled", streamDestinations[1].enabled);
	settings.setValue("window/geometry", main->saveGeometry());
	settings.setValue("window/layoutVersion", 14);
	if (dockWorkspace)
		settings.setValue("window/dockWorkspace", dockWorkspace->saveState(14));
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

void PresenterPanel::RenderTransmissionPreview(void *data, uint32_t cx, uint32_t cy)
{
	auto *panel = static_cast<PresenterPanel *>(data);
	if (!panel || !panel->transmissionTransition)
		return;
	obs_source_t *source = panel->transmissionTransition;
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

void PresenterPanel::UpdateTransmissionItemBounds()
{
	obs_video_info videoInfo = {};
	const bool hasVideoInfo = obs_get_video_info(&videoInfo);
	const float canvasWidth = hasVideoInfo ? float(videoInfo.base_width) : 1920.0f;
	const float canvasHeight = hasVideoInfo ? float(videoInfo.base_height) : 1080.0f;
	const vec2 fullSize = {canvasWidth, canvasHeight};
	const vec2 origin = {0.0f, 0.0f};
	auto setFullCanvas = [&fullSize, &origin](obs_sceneitem_t *item) {
		if (!item)
			return;
		obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_STRETCH);
		obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds(item, &fullSize);
		obs_sceneitem_set_pos(item, &origin);
	};
	setFullCanvas(transmissionPresenterOnlyItem);
	setFullCanvas(transmissionCameraOnlyItem);
	setFullCanvas(transmissionBackgroundItem);
	auto setCombinedBounds = [canvasWidth, canvasHeight](obs_sceneitem_t *item, double x, double y, double width,
							       double height) {
		if (!item)
			return;
		const vec2 bounds = {canvasWidth * float(width / 100.0), canvasHeight * float(height / 100.0)};
		const vec2 position = {canvasWidth * float(x / 100.0), canvasHeight * float(y / 100.0)};
		obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
		obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds(item, &bounds);
		obs_sceneitem_set_pos(item, &position);
	};
	setCombinedBounds(transmissionCameraItem, combinedCameraX, combinedCameraY, combinedCameraWidth,
			  combinedCameraHeight);
	setCombinedBounds(transmissionPresenterItem, combinedPresenterX, combinedPresenterY, combinedPresenterWidth,
			  combinedPresenterHeight);
}

obs_source_t *PresenterPanel::TransmissionSceneSource(TransmissionView view) const
{
	if (view == TransmissionView::Cameras)
		return transmissionCameraScene ? obs_scene_get_source(transmissionCameraScene) : nullptr;
	if (view == TransmissionView::CamerasAndPresenter)
		return transmissionScene ? obs_scene_get_source(transmissionScene) : nullptr;
	return transmissionPresenterScene ? obs_scene_get_source(transmissionPresenterScene) : nullptr;
}

void PresenterPanel::ApplyCombinedBackground()
{
	if (!transmissionScene)
		return;
	if (transmissionBackgroundItem) {
		obs_sceneitem_remove(transmissionBackgroundItem);
		transmissionBackgroundItem = nullptr;
	}
	transmissionBackgroundSource = nullptr;
	OBSDataAutoRelease settings = obs_data_create();
	QByteArray sourceId;
	if (combinedBackgroundType == QStringLiteral("image") && QFileInfo::exists(combinedBackgroundPath)) {
		sourceId = "image_source";
		obs_data_set_string(settings, "file", combinedBackgroundPath.toUtf8().constData());
	} else if (combinedBackgroundType == QStringLiteral("video") && QFileInfo::exists(combinedBackgroundPath)) {
		sourceId = "ffmpeg_source";
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_string(settings, "local_file", combinedBackgroundPath.toUtf8().constData());
		obs_data_set_bool(settings, "looping", combinedBackgroundLoop);
		obs_data_set_bool(settings, "clear_on_media_end", false);
		obs_data_set_bool(settings, "restart_on_activate", false);
	} else {
		combinedBackgroundType = QStringLiteral("color");
		sourceId = "color_source";
		QColor color(combinedBackgroundColor);
		if (!color.isValid())
			color = Qt::black;
		color.setAlpha(255);
		obs_data_set_int(settings, "color", ColorToObsInt(color));
		obs_data_set_int(settings, "width", 1920);
		obs_data_set_int(settings, "height", 1080);
	}
	transmissionBackgroundSource =
		obs_source_create_private(sourceId.constData(), "OPBS Both Background", settings);
	if (!transmissionBackgroundSource)
		return;
	obs_source_set_audio_mixers(transmissionBackgroundSource, 0u);
	obs_source_set_volume(transmissionBackgroundSource, 0.0f);
	transmissionBackgroundItem = obs_scene_add(transmissionScene, transmissionBackgroundSource);
	if (transmissionBackgroundItem)
		obs_sceneitem_set_order(transmissionBackgroundItem, OBS_ORDER_MOVE_BOTTOM);
	UpdateTransmissionItemBounds();
}

void PresenterPanel::ApplyTransmissionView(TransmissionView view, bool save)
{
	transmissionView = view;
	UpdateTransmissionItemBounds();
	obs_source_t *target = TransmissionSceneSource(view);
	if (transmissionTransition && target) {
		if (save)
			obs_transition_start(transmissionTransition, OBS_TRANSITION_MODE_AUTO,
					     uint32_t(transmissionTransitionDuration), target);
		else
			obs_transition_set(transmissionTransition, target);
	}
	if (camerasViewButton)
		camerasViewButton->setChecked(view == TransmissionView::Cameras);
	if (presenterViewButton)
		presenterViewButton->setChecked(view == TransmissionView::Presenter);
	if (combinedViewButton)
		combinedViewButton->setChecked(view == TransmissionView::CamerasAndPresenter);
	if (save && !restoring)
		SaveSettings();
}

void PresenterPanel::RefreshCameraSource()
{
	if (!transmissionScene || !transmissionCameraScene)
		return;
	if (transmissionCameraItem) {
		obs_sceneitem_remove(transmissionCameraItem);
		transmissionCameraItem = nullptr;
	}
	if (transmissionCameraOnlyItem) {
		obs_sceneitem_remove(transmissionCameraOnlyItem);
		transmissionCameraOnlyItem = nullptr;
	}
	cameraSource = nullptr;
	if (selectedCameraId.isEmpty()) {
		ApplyTransmissionView(transmissionView, false);
		return;
	}
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "video_device_id", selectedCameraId.toUtf8().constData());
	obs_data_set_int(settings, "res_type", 0);
	obs_data_set_bool(settings, "active", cameraEnabled);
	/* OPBS captura el audio de transmisión con una fuente WASAPI separada.
	 * Evitar que DirectShow abra también el pin de audio permite usar video
	 * y audio del mismo dispositivo USB sin competir por el endpoint. */
	obs_data_set_bool(settings, "opbs_disable_device_audio", true);
	cameraSource = obs_source_create_private("dshow_input", "OPBS Camera", settings);
	if (!cameraSource) {
		blog(LOG_WARNING, "OPBS could not create camera source '%s'", selectedCameraId.toUtf8().constData());
		return;
	}
	/* El audio de la cámara no forma parte de la mezcla OPBS. Solo se usan
	 * el puente del presentador y la entrada elegida en Transmisión > Audio. */
	obs_source_set_audio_mixers(cameraSource, 0u);
	obs_source_set_monitoring_type(cameraSource, OBS_MONITORING_TYPE_NONE);
	transmissionCameraItem = obs_scene_add(transmissionScene, cameraSource);
	transmissionCameraOnlyItem = obs_scene_add(transmissionCameraScene, cameraSource);
	if (transmissionPresenterItem)
		obs_sceneitem_set_order(transmissionPresenterItem, OBS_ORDER_MOVE_TOP);
	ApplyTransmissionView(transmissionView, false);
}

void PresenterPanel::SetCameraEnabled(bool enabled)
{
	cameraEnabled = enabled && !selectedCameraId.isEmpty();
	if (!cameraSource) {
		if (cameraEnabled)
			RefreshCameraSource();
		return;
	}
	calldata_t data = {0};
	calldata_set_bool(&data, "active", cameraEnabled);
	proc_handler_t *handler = obs_source_get_proc_handler(cameraSource);
	const bool called = handler && proc_handler_call(handler, "activate", &data);
	calldata_free(&data);
	if (!called && cameraEnabled)
		RefreshCameraSource();
	blog(LOG_INFO, "OPBS camera source %s: '%s'", cameraEnabled ? "activated" : "deactivated",
	     selectedCameraName.toUtf8().constData());
}

OBSServiceAutoRelease PresenterPanel::CreateStreamService(const StreamDestination &destination, const char *name) const
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "server", destination.server.toUtf8().constData());
	obs_data_set_string(settings, "key", destination.key.toUtf8().constData());
	if (destination.service == QStringLiteral("Personalizado"))
		return obs_service_create("rtmp_custom", name, settings, nullptr);
	obs_data_set_string(settings, "service",
			    destination.service == QStringLiteral("Facebook") ? "Facebook Live" : "YouTube - RTMPS");
	return obs_service_create("rtmp_common", name, settings, nullptr);
}

void PresenterPanel::ApplyPrimaryStreamService()
{
	if (!main || main->StreamingActive())
		return;
	OBSServiceAutoRelease service = CreateStreamService(streamDestinations[0], "OPBS Primary Service");
	if (!service)
		return;
	main->SetService(service);
	main->SaveService();
	config_t *profileConfig = main->Config();
	config_set_string(profileConfig, "Output", "Mode", "Simple");
	config_set_uint(profileConfig, "SimpleOutput", "VBitrate", streamVideoBitrate);
	config_set_uint(profileConfig, "SimpleOutput", "ABitrate", streamAudioBitrate);
	if (!recordingPath.isEmpty())
		config_set_string(profileConfig, "SimpleOutput", "FilePath", recordingPath.toUtf8().constData());
	config_save_safe(profileConfig, "tmp", nullptr);
}

void PresenterPanel::StartSecondaryStream()
{
	if (secondaryStreamStarting || !streamDestinations[1].enabled || streamDestinations[1].key.trimmed().isEmpty() ||
	    streamDestinations[1].server.trimmed().isEmpty())
		return;
	secondaryStreamStarting = true;
	OBSOutputAutoRelease primaryOutput = obs_frontend_get_streaming_output();
	if (!primaryOutput) {
		secondaryStreamStarting = false;
		return;
	}
	secondaryStreamService = CreateStreamService(streamDestinations[1], "OPBS Secondary Service");
	secondaryStreamOutput = obs_output_create("rtmp_output", "OPBS Secondary Stream", nullptr, nullptr);
	if (!secondaryStreamService || !secondaryStreamOutput) {
		secondaryStreamStarting = false;
		QMessageBox::warning(main, tr("Segundo destino"), tr("No fue posible preparar el segundo destino."));
		return;
	}
	obs_output_set_video_encoder(secondaryStreamOutput, obs_output_get_video_encoder(primaryOutput));
	obs_output_set_audio_encoder(secondaryStreamOutput, obs_output_get_audio_encoder(primaryOutput, 0), 0);
	obs_output_set_service(secondaryStreamOutput, secondaryStreamService);
	obs_output_set_reconnect_settings(secondaryStreamOutput, 25, 2);
	if (!obs_output_start(secondaryStreamOutput)) {
		const char *error = obs_output_get_last_error(secondaryStreamOutput);
		QMessageBox::warning(main, tr("Segundo destino"),
				     tr("La transmisión principal inició, pero el segundo destino falló: %1")
					     .arg(QString::fromUtf8(error ? error : "Error desconocido")));
	}
	secondaryStreamStarting = false;
}

void PresenterPanel::StopSecondaryStream()
{
	if (secondaryStreamOutput && obs_output_active(secondaryStreamOutput))
		obs_output_force_stop(secondaryStreamOutput);
	secondaryStreamOutput = nullptr;
	secondaryStreamService = nullptr;
	secondaryStreamStarting = false;
}

void PresenterPanel::ToggleStreaming()
{
	if (main->StreamingActive()) {
		StopSecondaryStream();
		main->StopStreaming();
		return;
	}
	const StreamDestination &primary = streamDestinations[0];
	if (primary.server.trimmed().isEmpty() || primary.key.trimmed().isEmpty()) {
		QMessageBox::information(main, tr("Configurar transmisión"),
					 tr("Abre Transmisión y completa el servidor y la clave del primer destino."));
		UpdateTransmissionButtons();
		return;
	}
	if (!transmissionPresenterAudioSource ||
	    (!selectedTransmissionInputId.isEmpty() && !transmissionInputSource)) {
		QMessageBox::warning(main, tr("Audio de transmisión"),
				     tr("No fue posible preparar una de las dos entradas de audio. Revisa "
					"Transmisión > Audio antes de emitir."));
		return;
	}
	ApplyPrimaryStreamService();
	main->StartStreaming();
}

void PresenterPanel::ToggleRecording()
{
	if (main->RecordingActive())
		main->StopRecording();
	else {
		if (!transmissionPresenterAudioSource ||
		    (!selectedTransmissionInputId.isEmpty() && !transmissionInputSource)) {
			QMessageBox::warning(main, tr("Audio de grabación"),
					     tr("No fue posible preparar una de las entradas de audio configuradas."));
			return;
		}
		ApplyPrimaryStreamService();
		main->StartRecording();
	}
}

void PresenterPanel::UpdateTransmissionButtons()
{
	const bool streaming = main && main->StreamingActive();
	const bool recording = main && main->RecordingActive();
	if (streamButton) {
		streamButton->setChecked(streaming);
		streamButton->setText(streaming ? tr("Finalizar") : tr("Transmitir"));
	}
	if (recordButton) {
		recordButton->setChecked(recording);
		recordButton->setText(recording ? tr("Detener") : tr("Grabar"));
	}
}
