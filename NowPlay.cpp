/*
    File: nowplay.cpp
    Created on: 01/01/2020
    Author: Felix de las Pozas Alvarez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// C++
#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>

// Project
#include "NowPlay.h"
#include "version.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"

// Qt
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QString>
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QPainter>

// Win64 builds
#ifdef __WIN64__
#include <windef.h>
#include "winampapi.h"
#include <QWinTaskbarProgress>
#endif

const QString GEOMETRY      = "Geometry";
const QString FOLDER        = "Folder";
const QString COPYSIZE      = "Copy Size";
const QString COPYUNITS     = "Copy Units";
const QString DESTINATION   = "Destination Folder";
const QString USE_AUDPLAYER = "Play In Music player";
const QString USE_VIDPLAYER = "Play In Video player";
const QString SUBTITLESIZE  = "Subtitle Size";
const QString AUDPLAYER_LOC = "Music player location";
const QString VIDPLAYER_LOC = "Video player location";
const QString CASTNOW_LOC   = "Castnow Location";
const QString THEME         = "Application Theme";
const QString CONTINUOUS    = "Continuous Play";

const unsigned long long MEGABYTE = 1024*1024;

//-----------------------------------------------------------------------------
NowPlay::NowPlay()
: QDialog     {nullptr}
, m_process   {this}
, m_command   {this}
, m_continuous{false}
, m_icon      {new QSystemTrayIcon(QIcon(":/NowPlay/buttons.svg"), this)}
, m_thread    {nullptr}
#ifdef __WIN64__
, m_taskBarButton{nullptr}
#endif
{
  setWindowFlags(Qt::WindowFlags() & Qt::Dialog & Qt::WindowMinimizeButtonHint & ~Qt::WindowContextHelpButtonHint);

  setupUi(this);

  setupTrayIcon();

  loadSettings();

  updateGUI();

  checkApplications();

  connectSignals();
}

//-----------------------------------------------------------------------------
NowPlay::~NowPlay()
{
  saveSettings();
}

//-----------------------------------------------------------------------------
void NowPlay::loadSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "NowPlay");

  if(settings.contains(GEOMETRY))
  {
    auto geometry = settings.value(GEOMETRY).toByteArray();
    restoreGeometry(geometry);
  }

  const auto baseDir = settings.value(FOLDER, QApplication::applicationDirPath()).toString();

  m_baseDir->setText(QDir::toNativeSeparators(baseDir));

  const auto destinationDir = settings.value(DESTINATION, QApplication::applicationDirPath()).toString();

  m_destinationDir->setText(QDir::toNativeSeparators(destinationDir));

  const auto copyAmount = settings.value(COPYSIZE, 0).toInt();

  m_amount->setCurrentIndex(copyAmount);

  const auto copyUnits = settings.value(COPYUNITS, 0).toInt();

  m_units->setCurrentIndex(copyUnits);

  const auto useAudio = settings.value(USE_AUDPLAYER, false).toBool();
  const auto useVideo = settings.value(USE_VIDPLAYER, false).toBool();

  m_useMusicPlayer->setChecked(useAudio);
  m_useVideoPlayer->setChecked(useVideo);
  m_castnow->setChecked(!useAudio && !useVideo);
  m_play->setEnabled(useAudio || useVideo || m_castnow->isChecked());

  const auto subtitleSize = settings.value(SUBTITLESIZE, 1.3).toDouble();

  m_subtitleSizeSlider->setValue(subtitleSize * 10);
  onSubtitleSizeChanged(subtitleSize * 10);

  m_musicPlayerPath = settings.value(AUDPLAYER_LOC, "").toString();
  m_videoPlayerPath = settings.value(VIDPLAYER_LOC, "").toString();
  m_castnowPath  = settings.value(CASTNOW_LOC,  "").toString();

  m_continuous = settings.value(CONTINUOUS, false).toBool();

  const auto theme = settings.value(THEME, QString()).toString();

  if(theme.compare("dark") == 0)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    qApp->setStyleSheet(ts.readAll());
  }
}

//-----------------------------------------------------------------------------
void NowPlay::saveSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "NowPlay");

  settings.setValue(GEOMETRY,      saveGeometry());
  settings.setValue(FOLDER,        m_baseDir->text());
  settings.setValue(DESTINATION,   m_destinationDir->text());
  settings.setValue(COPYSIZE,      m_amount->currentIndex());
  settings.setValue(COPYUNITS,     m_units->currentIndex());
  settings.setValue(USE_AUDPLAYER, m_useMusicPlayer->isChecked());
  settings.setValue(USE_VIDPLAYER, m_useVideoPlayer->isChecked());
  settings.setValue(SUBTITLESIZE,  static_cast<double>(m_subtitleSizeSlider->value()/10.));
  settings.setValue(AUDPLAYER_LOC, m_musicPlayerPath);
  settings.setValue(VIDPLAYER_LOC, m_videoPlayerPath);
  settings.setValue(CASTNOW_LOC,   m_castnowPath);
  settings.setValue(CONTINUOUS,    m_continuous);
  settings.setValue(THEME,         qApp->styleSheet().isEmpty() ? QString():"dark");

  settings.sync();
}

//-----------------------------------------------------------------------------
bool NowPlay::callWinamp()
{
#ifdef __WIN64__
  if(!Utils::checkIfValidMusicPlayerLocation(m_musicPlayerPath)) return false;
  if(!m_musicPlayerPath.endsWith("winamp.exe", Qt::CaseInsensitive)) return false;
  auto handler = WinAmp::getWinAmpHandle(m_musicPlayerPath);

  if(!handler)
  {
    showErrorMessage(tr("Unable to launch or contact WinAmp"));
    return false;
  }

  WinAmp::deletePlaylist(handler);

  auto isPlaylist = [this](const Utils::FileInformation &f){ return Utils::isPlaylistFile(f.first); };
  auto it = std::find_if(m_files.cbegin(), m_files.cend(), isPlaylist);
  if(it != m_files.cend())
  {
    const auto filename = QString::fromStdWString((*it).first.wstring());
    WinAmp::addFile(handler, QDir::toNativeSeparators(filename).toStdWString());
  }
  else
  {
    auto checkAndAdd = [&](const Utils::FileInformation &f)
    {
      if(Utils::isAudioFile(f.first))
      {
        const auto filename = QString::fromStdWString(f.first.wstring());
        WinAmp::addFile(handler, QDir::toNativeSeparators(filename).toStdWString());
        return true;
      }

      return false;
    };
    auto count = std::count_if(m_files.cbegin(), m_files.cend(), checkAndAdd);

    if(count == 0)
    {
      const auto message = tr("No playable files found in directory: ") + QString::fromStdWString(m_files.front().first.parent_path().wstring());
      showErrorMessage(message);
      return false;
    }
  }

  m_files.clear();

  WinAmp::startPlay(handler);

  return true;
#endif
}

//-----------------------------------------------------------------------------
void NowPlay::playVideos()
{
  if(!Utils::checkIfValidVideoPlayerLocation(m_videoPlayerPath)) return;

  QStringList arguments;
  arguments << "-no-close-at-end";
  arguments << "-add-to-playlist";

  auto addToArguments = [&arguments](const Utils::FileInformation &f)
  {
    if(Utils::isVideoFile(f.first))
    {
      arguments << QString::fromStdWString(f.first.wstring());
    }
  };
  std::for_each(m_files.cbegin(), m_files.cend(), addToArguments);

  m_process.startDetached(m_videoPlayerPath, arguments);

  m_files.clear();
}

//-----------------------------------------------------------------------------
void NowPlay::castFile()
{
  if(m_process.state() == QProcess::Running)
  {
    m_process.blockSignals(true);

    sendCommand("s");
    sendCommand("quit");

    m_process.kill();
    m_process.waitForFinished(-1);
    m_files.clear();

    m_process.blockSignals(false);

    resetState();

    return;
  }

  if(!m_castnow->isChecked() || !Utils::checkIfValidCastnowLocation(m_castnowPath)) return;

  auto isValidFile = [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); };
  auto file = std::find_if(m_files.begin(), m_files.end(), isValidFile);
  if(file != m_files.end())
  {
    const auto filename = (*file).first;
    m_files.erase(file);

    const auto hasMoreFiles = (!m_files.empty() && std::count_if(m_files.cbegin(), m_files.cend(), isValidFile) > 0);

    m_play->setText("Stop");
    m_next->setEnabled(hasMoreFiles || m_continuous);
    m_icon->contextMenu()->actions().at(1)->setText("Stop");
    m_icon->contextMenu()->actions().at(2)->setEnabled(hasMoreFiles || m_continuous);

    setProgress(m_progress->value() + 1);

    log(tr("Playing %1/%2 - ").arg(m_progress->value()).arg(m_progress->maximum()) + QString::fromStdWString(filename.filename().wstring()));

    QStringList arguments;
    arguments << QString::fromStdWString(filename.wstring());
    if(Utils::isVideoFile(filename.string()))
    {
      arguments << "--subtitle-scale";
      arguments << m_subtitleSizeLabel->text();
    }

    m_process.start(m_castnowPath, arguments, QProcess::Unbuffered|QProcess::ReadWrite);
    m_process.waitForStarted();

    const auto title   = QString::fromStdWString(filename.parent_path().filename().wstring());
    const auto message = QString::fromStdWString(filename.filename().wstring()) + tr(" (%1/%2)").arg(m_progress->value()).arg(m_progress->maximum());
	
    if(m_icon->isVisible())
    {
      const auto pIcon = progressIcon();
#ifdef __WIN64__
      m_icon->showMessage(title, message, pIcon, 7500);
#else
      m_icon->showMessage(title, message, QSystemTrayIcon::Information, 7500);
#endif
      m_icon->setIcon(pIcon);
    }
	
    m_icon->setToolTip(title + tr("\n") + message);
  }
  else
  {
    m_files.clear();

    if(m_continuous)
    {
      onPlayButtonClicked();
    }
    else
    {
      resetState();
    }
  }
}

//-----------------------------------------------------------------------------
void NowPlay::connectSignals()
{
  connect(m_tabWidget,          SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
  connect(m_browseBase,         SIGNAL(pressed()),           this, SLOT(browseDir()));
  connect(m_browseDestination,  SIGNAL(pressed()),           this, SLOT(browseDir()));
  connect(m_about,              SIGNAL(pressed()),           this, SLOT(onAboutButtonClicked()));
  connect(m_play,               SIGNAL(pressed()),           this, SLOT(onPlayButtonClicked()));
  connect(m_next,               SIGNAL(pressed()),           this, SLOT(playNext()));
  connect(m_exit,               SIGNAL(pressed()),           this, SLOT(close()));
  connect(m_settings,           SIGNAL(pressed()),           this, SLOT(onSettingsButtonClicked()));
  connect(m_subtitleSizeSlider, SIGNAL(valueChanged(int)),   this, SLOT(onSubtitleSizeChanged(int)));

  connect(&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onOuttputAvailable()));
  connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(castFile()));

  connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

//-----------------------------------------------------------------------------
void NowPlay::onTabChanged(int index)
{
  const QString text = (index == 0) ? "Now Play!" : "Now Copy!";
  m_play->setText(text);
}

//-----------------------------------------------------------------------------
void NowPlay::onPlayButtonClicked()
{
  if(m_process.state() == QProcess::Running)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_process.blockSignals(true);

    sendCommand("s");
    sendCommand("quit");

    m_process.kill();
    m_process.waitForFinished(-1);
    m_files.clear();

    m_process.blockSignals(false);

    resetState();

    QApplication::restoreOverrideCursor();

    return;
  }

  if(!m_files.empty())
  {
    QMessageBox::Button button;

    if(!(m_continuous && m_castnow->isChecked()))
    {
      QMessageBox msgBox(this);
      msgBox.setWindowIcon(QIcon(":/NowPlay/buttons.svg"));
      msgBox.setWindowTitle(tr("Now Play!"));
      msgBox.setText(tr("%1 files are on the playlist. Do you want to replace or play the current playlist?").arg(m_files.size()));
      msgBox.setIcon(QMessageBox::Icon::Information);
      msgBox.setStandardButtons(QMessageBox::Button::Cancel|QMessageBox::Button::Ok);
      msgBox.button(QMessageBox::Button::Cancel)->setText("Play");
      msgBox.button(QMessageBox::Button::Ok)->setText("Replace");

      button = static_cast<QMessageBox::Button>(msgBox.exec());
    }
    else
    {
      button = QMessageBox::Button::Cancel;
    }

    switch(button)
    {
      case QMessageBox::Button::Ok:
        m_files.clear();
        break;
      default:
      case QMessageBox::Button::Cancel:
        {
          if(m_castnow->isChecked())
          {
            const auto count = std::count_if(m_files.cbegin(), m_files.cend(), [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); });

            setProgressRange(0, count);

            setProgress(0);

            m_tabWidget->setEnabled(false);

            castFile();
          }
          else
          {
            if(m_useVideoPlayer->isChecked())
            {
              playVideos();
            }
            else
            {
              callWinamp();
            }
          }
          return;
        }
        break;
    }
  }

  const bool isCopyMode = m_tabWidget->currentIndex() == 1;

  std::filesystem::path directory = QDir::fromNativeSeparators(m_baseDir->text()).toStdWString();
  auto validPaths = Utils::getSubdirectories(directory, isCopyMode);

  // Copy mode
  if(isCopyMode)
  {
    if(m_thread)
    {
      QMessageBox msgBox(this);
      msgBox.setWindowIcon(QIcon(":/NowPlay/buttons.svg"));
      msgBox.setWindowTitle(tr("Now Play!"));
      msgBox.setText(tr("Already copying files! Do you want to stop the process?"));
      msgBox.setIcon(QMessageBox::Icon::Information);
      msgBox.setStandardButtons(QMessageBox::Button::No|QMessageBox::Button::Yes);

      if(QMessageBox::Button::Yes == msgBox.exec())
      {
        if(m_thread) m_thread->stop();
      }

      return;
    }

    const auto destination = m_destinationDir->text().toStdWString();
    bool ok = false;
    auto size = m_amount->currentText().toInt(&ok, 10);

    if(!ok)
    {
      showErrorMessage(tr("Invalid size option value."));
      return;
    }

    if(destination.empty() || !std::filesystem::exists(destination) || !std::filesystem::is_directory(destination))
    {
      showErrorMessage(tr("No destination directory to copy to."));
      return;
    }

    if (validPaths.empty())
    {
      showErrorMessage(tr("No sub-directories to select from."));
      return;
    }

    switch(m_units->currentIndex())
    {
      case 1:
        size *= MEGABYTE;
        break;
      case 2:
        size *= MEGABYTE * 1024;
        break;
      case 0:
      default:
        break;
    }

    QString message = tr("Selecting from base for ") + QString::number(size) + " bytes...";
    log(message);

    auto selectedDirs = Utils::getCopyDirectories(validPaths, size);

    if(!selectedDirs.empty())
    {
      m_thread = std::make_shared<CopyThread>(selectedDirs, destination, this);

      connect(m_thread.get(), SIGNAL(log(const QString &)), this, SLOT(log(const QString &)));
      connect(m_thread.get(), SIGNAL(progress(const int)), this, SLOT(setProgress(const int)));
      connect(m_thread.get(), SIGNAL(finished()), this, SLOT(onCopyFinished()));

      m_play->setText("Stop");
      m_tabWidget->setEnabled(false);
      QApplication::setOverrideCursor(Qt::WaitCursor);

      m_thread->start();

      return;
    }
    else
    {
      const auto message = tr("Unable to select directories for the given size: ") + QString::number(size) + " " + m_units->currentText() + ".";
      showErrorMessage(message);
      return;
    }

    return;
  }

  // Play mode.
  if(!validPaths.empty())
  {
    QString message = tr("<b>") + QDir::toNativeSeparators(QString::fromStdWString(directory.wstring())) + tr("</b> has ") + QString::number(validPaths.size()) + tr(" directories.");
    log(message);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);

    std::uniform_int_distribution<int> distribution(1, validPaths.size());
    int roll = distribution(generator);

    directory = validPaths.at(roll-1).first;

    message = QString("Selected: <b>") + QString::fromStdWString(directory.filename().wstring()) + tr("</b>");
    log(message);
  }
  else
  {
    QString message = QString("Base directory: <b>") + QDir::toNativeSeparators(QString::fromStdWString(directory.wstring())) + tr("</b>");
    log(message);
  }

  auto files = Utils::getPlayableFiles(directory);
  std::move(files.begin(), files.end(), std::back_inserter(m_files));

  if(!m_files.empty())
  {
    if(m_useMusicPlayer->isChecked())
    {
      if(!callWinamp())
      {
        playAudio();
      }
    }
    else
    {
      if(m_castnow->isChecked())
      {
        const auto count = std::count_if(m_files.cbegin(), m_files.cend(), [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); });

        setProgressRange(0, count);

        setProgress(0);

        m_tabWidget->setEnabled(false);

        castFile();
      }
      else
      {
        playVideos();
      }
    }
  }
  else
  {
    const auto message = QString("No music files found in directory: ") + QString::fromStdWString(directory.wstring());
    showErrorMessage(message);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::onAboutButtonClicked()
{
  AboutDialog dialog(this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void NowPlay::browseDir()
{
  auto origin = qobject_cast<QToolButton *>(sender());
  QString title;
  QString dir;
  QLineEdit *widget = nullptr;

  if(origin == m_browseBase)
  {
    title = tr("Select base directory");
    dir = m_baseDir->text();
    widget = m_baseDir;
  }
  else
  {
    title = tr("Select destination directory");
    dir = m_destinationDir->text();
    widget = m_destinationDir;
  }

  auto newDirectory = QFileDialog::getExistingDirectory(this, title, dir);

  if(!newDirectory.isEmpty()) widget->setText(QDir::toNativeSeparators(newDirectory));

  if(origin) origin->setDown(false);
}

//-----------------------------------------------------------------------------
void NowPlay::showErrorMessage(const QString &message, const QString &title, const QString &details)
{
  QMessageBox msgBox(this);
  msgBox.setWindowIcon(QIcon(":/NowPlay/buttons.svg"));
  msgBox.setWindowTitle(title);
  msgBox.setText(message);
  if(!details.isEmpty()) msgBox.setDetailedText(details);
  msgBox.setIcon(QMessageBox::Icon::Critical);
  msgBox.exec();
}

//-----------------------------------------------------------------------------
void NowPlay::log(const QString &message)
{
  m_log->append(message.toUtf8());
}

//-----------------------------------------------------------------------------
void NowPlay::keyPressEvent(QKeyEvent *e)
{
  if(e->key() == Qt::Key_Escape)
  {
    e->accept();
    hide();
    m_icon->show();
    updateTrayIcon();
    return;
  }

  QString command;
  switch(e->key())
  {
    case Qt::Key_Up:
      command = "up";
      break;
    case Qt::Key_Down:
      command = "down";
      break;
    case Qt::Key_S:
      command = "s";
      break;
    case Qt::Key_Q:
      command = "quit";
      break;
    case Qt::Key_Left:
      command = "left";
      break;
    case Qt::Key_Right:
      command = "right";
      break;
    case Qt::Key_Space:
      command = "space";
      break;
    case Qt::Key_M:
      command = "m";
      break;
    case Qt::Key_T:
      command = "t";
      break;
    default:
      break;
  }

  if(!command.isEmpty())
  {
    sendCommand(command);
  }
  else
  {
    QDialog::keyPressEvent(e);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::updateGUI()
{
  m_tabWidget->setCurrentIndex(0);
  m_next->setEnabled(false);
  m_icon->contextMenu()->actions().at(1)->setText("Now Play!");
  m_icon->contextMenu()->actions().at(2)->setEnabled(false);
  m_icon->setIcon(QIcon(":/NowPlay/buttons.svg"));

  setAcceptDrops(true);
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

//-----------------------------------------------------------------------------
void NowPlay::checkApplications()
{
  const auto validMusicPlayer = Utils::checkIfValidMusicPlayerLocation(m_musicPlayerPath);
  const auto validVideoPlayer = Utils::checkIfValidVideoPlayerLocation(m_videoPlayerPath);
  const auto validCastnow     = Utils::checkIfValidCastnowLocation(m_castnowPath);

  m_useMusicPlayer->setEnabled(validMusicPlayer);
  m_useVideoPlayer->setEnabled(validVideoPlayer);
  m_castnow->setEnabled(validCastnow);
  m_subtitleSizeSlider->setEnabled(validCastnow);

  const auto isValid = validMusicPlayer || validVideoPlayer || validCastnow;
  m_play->setEnabled(isValid);
}

//-----------------------------------------------------------------------------
bool NowPlay::event(QEvent *event)
{
  if(event->type() == QEvent::KeyPress)
  {
    auto ke = static_cast<QKeyEvent *>(event);
    keyPressEvent(ke);
    return true;
  }

  return QDialog::event(event);
}

//-----------------------------------------------------------------------------
void NowPlay::onOuttputAvailable()
{
  const auto data = QString(m_process.readAll()).toStdString();
  auto pos = data.find("Idle...");
  if(std::string::npos != pos)
  {
    m_process.kill();
    m_process.waitForFinished(-1);
    return;
  }

  pos = data.find("Error: Load failed");
  if(std::string::npos != pos)
  {
    log(tr("<b><font color =\"red\">Unable to play!</font></b>"));

    m_process.kill();
    m_process.waitForFinished(-1);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::onSubtitleSizeChanged(int value)
{
  auto numberText = QString::number(static_cast<double>(value/10.), 'e', 2);
  m_subtitleSizeLabel->setText(numberText.mid(0,3));
}

//-----------------------------------------------------------------------------
void NowPlay::playNext()
{
  m_process.kill();
  m_process.waitForFinished(-1);
}

//-----------------------------------------------------------------------------
void NowPlay::onSettingsButtonClicked()
{
  const auto theme = qApp->styleSheet();

  SettingsDialog::PlayConfiguration config;
  config.musicPlayerPath = m_musicPlayerPath;
  config.videoPlayerPath = m_videoPlayerPath;
  config.castnowPath = m_castnowPath;
  config.continuous = m_continuous;

  SettingsDialog dialog(config, this);
  if(QDialog::Accepted == dialog.exec())
  {
    m_musicPlayerPath = dialog.getMusicPlayerLocation();
    m_videoPlayerPath = dialog.getVideoPlayerLocation();
    m_castnowPath = dialog.getCastnowLocation();
    m_continuous = dialog.getContinuousPlay();

    checkApplications();
  }
  else
  {
    qApp->setStyleSheet(theme);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if(reason == QSystemTrayIcon::DoubleClick)
  {
    onRestoreActionActivated();
  }
}

//-----------------------------------------------------------------------------
void NowPlay::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::WindowStateChange)
  {
    if (isMinimized())
    {
      hide();

      m_icon->show();
      updateTrayIcon();
      e->ignore();
    }
  }
}

//-----------------------------------------------------------------------------
void NowPlay::closeEvent(QCloseEvent *e)
{
  QWidget::closeEvent(e);

  emit terminated();
}

//-----------------------------------------------------------------------------
void NowPlay::setupTrayIcon()
{
  auto menu = new QMenu();

  auto restore = new QAction(QIcon(":/NowPlay/buttons.svg"), tr("Restore"), this);
  connect(restore, SIGNAL(triggered()),
          this,    SLOT(onRestoreActionActivated()));

  auto play = new QAction("Now Play!", this);
  connect(play, SIGNAL(triggered()),
          this, SLOT(onPlayButtonClicked()));

  auto next = new QAction(tr("Next"), this);
  connect(next, SIGNAL(triggered()),
          this, SLOT(playNext()));

  next->setEnabled(false);

  auto settings = new QAction(tr("Settings..."), this);
  connect(settings, SIGNAL(triggered()),
          this,     SLOT(onSettingsButtonClicked()));

  auto about = new QAction(tr("About..."), this);
  connect(about, SIGNAL(triggered()),
          this,  SLOT(onAboutButtonClicked()));

  auto quit = new QAction(tr("Exit"), this);
  connect(quit, SIGNAL(triggered()),
          this, SLOT(close()));

  menu->addAction(restore);
  menu->addAction(play);
  menu->addAction(next);
  menu->addSeparator();
  menu->addAction(settings);
  menu->addAction(about);
  menu->addAction(quit);

  m_icon->setContextMenu(menu);
  m_icon->setToolTip(tr("Now Play!"));
  m_icon->hide();
}

//-----------------------------------------------------------------------------
void NowPlay::updateTrayIcon()
{
  QIcon icon;

  if(m_process.state() == QProcess::Running)
  {
    icon = progressIcon();
  }
  else
  {
    icon = QIcon(":/NowPlay/buttons.svg");
  }

  m_icon->setIcon(icon);
}

//-----------------------------------------------------------------------------
void NowPlay::onRestoreActionActivated()
{
  m_icon->hide();

  setProgress(m_progress->value());

  showNormal();
}

//-----------------------------------------------------------------
void NowPlay::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

#ifdef __WIN64__
  if(!m_taskBarButton)
  {
    m_taskBarButton = new QWinTaskbarButton(this);
    m_taskBarButton->setWindow(this->windowHandle());
    m_taskBarButton->progress()->setVisible(false);
    m_taskBarButton->progress()->setValue(0);
  }
  else
 {
    setProgress(m_progress->value());
 }
#endif
}

//-----------------------------------------------------------------
void NowPlay::setProgress(const int value)
{
  if(!m_progress->isEnabled() && value != 0)
  {
    m_progress->setEnabled(true);
  }

  m_progress->setValue(value);

  if(value == 0)
  {
    m_progress->setEnabled(false);

    m_icon->setToolTip((m_tabWidget->currentIndex() == 0) ? tr("Now Play!") : tr("Now Copy!"));
  }

#ifdef __WIN64__
  if(!isMinimized())
  {
    if(!m_taskBarButton->progress()->isVisible() && value != 0)  m_taskBarButton->progress()->setVisible(true);
    m_taskBarButton->progress()->setValue(value);
  }

  if(value == 0 && !isMinimized())
  {
    m_taskBarButton->progress()->setVisible(false);
  }
#endif

  updateTrayIcon();
}

//-----------------------------------------------------------------------------
void NowPlay::dropEvent(QDropEvent *e)
{
  std::vector<Utils::FileInformation> files;

  if(e)
  {
    const auto data = e->mimeData();

    if (data->hasUrls())
    {
      const auto fileList = data->urls();

      for(auto filePath: fileList)
      {
        auto file = std::filesystem::path(filePath.toLocalFile().toStdWString());
        if(std::filesystem::is_regular_file(file) && (Utils::isAudioFile(file) || Utils::isVideoFile(file)))
        {
          files.emplace_back(file,0);
        }
      }

      if(!files.empty())
      {
        if(!m_files.empty())
        {
          QMessageBox msgBox(this);
          msgBox.setWindowIcon(QIcon(":/NowPlay/buttons.svg"));
          msgBox.setWindowTitle(tr("Now Play!"));
          msgBox.setText(tr("The current playlist has %1 pending files, and %2 files can be added to the playlist.\nDo you want to replace or merge with the current playlist?").arg(m_files.size()).arg(files.size()));
          msgBox.setIcon(QMessageBox::Icon::Information);
          msgBox.setStandardButtons(QMessageBox::Button::Ok|QMessageBox::Button::Abort|QMessageBox::Button::Cancel);
          msgBox.button(QMessageBox::Button::Abort)->setText("Merge");
          msgBox.button(QMessageBox::Button::Ok)->setText("Replace");

          const auto button = msgBox.exec();

          switch(button)
          {
            case QMessageBox::Button::Cancel:
              e->accept();
              return;
              break;
            case QMessageBox::Button::Ok: // Replace
              m_files.clear();
              if(m_process.state() == QProcess::Running)
              {
                setProgressRange(0, 1);
                setProgress(1); // 1 -> file currently playing to take into consideration later to compute total progress limits.
              }
              break;
            default:
            case QMessageBox::Button::Abort: // Merge
              break;
          }
        }

        std::sort(files.begin(), files.end(), Utils::lessThan);

        auto addFileToPlaylist = [this](const Utils::FileInformation &f)
        {
          log(tr("Added to current playlist: %1").arg(QString::fromStdWString(f.first.filename().wstring())));
          m_files.push_back(std::move(f));
        };
        std::for_each(files.cbegin(), files.cend(), addFileToPlaylist);

        auto isValidFile = [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); };
        const auto validFilesCount = std::count_if(m_files.cbegin(), m_files.cend(), isValidFile);

        const auto isCasting = (m_process.state() == QProcess::Running);
        if(isCasting)
        {
          const auto hasMoreFiles = (!m_files.empty() && validFilesCount > 0);

          m_next->setEnabled(hasMoreFiles);
          m_icon->contextMenu()->actions().at(2)->setEnabled(hasMoreFiles);

          const auto currentProgress =  m_progress->value();
          setProgressRange(0, currentProgress + validFilesCount);
          setProgress(currentProgress);
        }
      }
    }
  }
  e->setAccepted(!files.empty());
}

//-----------------------------------------------------------------------------
void NowPlay::dragEnterEvent(QDragEnterEvent *e)
{
  if (e->mimeData()->hasUrls())
  {
    e->acceptProposedAction();
  }
}

//-----------------------------------------------------------------------------
void NowPlay::resetState()
{
  setProgress(0);

  m_tabWidget->setEnabled(true);
  m_play->setText("Now Play!");
  m_next->setEnabled(false);
  m_icon->contextMenu()->actions().at(1)->setText("Now Play!");
  m_icon->contextMenu()->actions().at(2)->setEnabled(false);
  m_icon->setIcon(QIcon(":/NowPlay/buttons.svg"));
}

//-----------------------------------------------------------------------------
void NowPlay::onCopyFinished()
{
  auto thread = qobject_cast<CopyThread *>(sender());
  if(thread)
  {
    QApplication::restoreOverrideCursor();
    m_tabWidget->setEnabled(true);
    m_play->setText(tr("Now Copy!"));
    setProgress(0);

    auto error = thread->errorMessage();

    QMessageBox msgBox(this);
    msgBox.setWindowIcon(QIcon(":/NowPlay/buttons.svg"));
    msgBox.setWindowTitle(tr("Now Play!"));

    if(!error.isEmpty())
    {
      msgBox.setText(error);
      msgBox.setIcon(QMessageBox::Icon::Critical);
    }
    else
    {
      msgBox.setText(tr("Copy finished!"));
      msgBox.setIcon(QMessageBox::Icon::Information);
    }

    msgBox.setStandardButtons(QMessageBox::Button::Ok);
    msgBox.exec();

    m_thread = nullptr;
  }
}

//-----------------------------------------------------------------------------
void NowPlay::setProgressRange(const int minimum, const int maximum)
{
  m_progress->setRange(minimum, maximum);
#ifdef __WIN64__
  m_taskBarButton->progress()->setRange(minimum,  maximum);
#endif
}

//-----------------------------------------------------------------------------
QIcon NowPlay::progressIcon()
{
  auto image = QIcon(":/NowPlay/buttons.svg").pixmap(255, 255, QIcon::Mode::Disabled, QIcon::State::On);

  if(m_progress->maximum() != 0)
  {
    const double progressValue = static_cast<double>(m_progress->value()) / m_progress->maximum();
    const int percent = std::floor(255 * progressValue);
    const QRect rect(0, 0, percent, 255);
    const auto colorImage = QIcon(":/NowPlay/buttons.svg").pixmap(255, 255, QIcon::Mode::Normal, QIcon::State::On).copy(rect);

    QPainter painter;
    painter.begin(&image);
    painter.drawPixmap(rect, colorImage);
    painter.end();
  }

  return QIcon(image);
}

//-----------------------------------------------------------------------------
void NowPlay::sendCommand(const QString &command)
{
  if(!command.isEmpty() && m_castnow->isChecked() && (m_process.state() == QProcess::Running))
  {
    m_command.startDetached(m_castnowPath, QStringList{"--command", command,"--exit"});
    m_command.waitForFinished(-1);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::playAudio()
{
  if(!Utils::checkIfValidMusicPlayerLocation(m_musicPlayerPath)) return;

  QStringList arguments;

  auto addToArguments = [&arguments](const Utils::FileInformation &f)
  {
    if(Utils::isPlaylistFile(f.first))
    {
      arguments << QString::fromStdWString(f.first.wstring());
    }
  };
  std::for_each(m_files.cbegin(), m_files.cend(), addToArguments);

  m_process.startDetached(m_musicPlayerPath, arguments);

  m_files.clear();
}
