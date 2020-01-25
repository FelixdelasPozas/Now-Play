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
#include <windef.h>

// Project
#include "NowPlay.h"
#include "version.h"
#include "winampapi.h"
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
#include <QWinTaskbarProgress>

// Boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

const QString GEOMETRY     = "Geometry";
const QString FOLDER       = "Folder";
const QString COPYSIZE     = "Copy Size";
const QString COPYUNITS    = "Copy Units";
const QString DESTINATION  = "Destination Folder";
const QString USEWINAMP    = "Play In Winamp";
const QString USESMPLAYER  = "Play In SMPlayer";
const QString SUBTITLESIZE = "Subtitle Size";
const QString WINAMP_LOC   = "WinAmp Location";
const QString SMPLAYER_LOC = "SMPlayer Location";
const QString CASTNOW_LOC  = "Castnow Location";

const unsigned long long MEGABYTE = 1024*1024;

//-----------------------------------------------------------------------------
NowPlay::NowPlay()
: QDialog  {nullptr}
, m_process{this}
, m_icon   {new QSystemTrayIcon(QIcon(":/NowPlay/buttons.svg"), this)}
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

  const auto useWinamp = settings.value(USEWINAMP, false).toBool();
  const auto useSMPlyer = settings.value(USESMPLAYER, false).toBool();

  m_winamp->setChecked(useWinamp);
  m_smplayer->setChecked(useSMPlyer);
  m_castnow->setChecked(!useWinamp && !useSMPlyer);

  const auto subtitleSize = settings.value(SUBTITLESIZE, 1.3).toDouble();

  m_subtitleSizeSlider->setValue(subtitleSize * 10);
  onSubtitleSizeChanged(subtitleSize * 10);

  m_winampPath   = settings.value(WINAMP_LOC,   QDir::home().absolutePath()).toString();
  m_smplayerPath = settings.value(SMPLAYER_LOC, QDir::home().absolutePath()).toString();
  m_castnowPath  = settings.value(CASTNOW_LOC,  QDir::home().absolutePath()).toString();
}

//-----------------------------------------------------------------------------
void NowPlay::saveSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "NowPlay");

  settings.setValue(GEOMETRY,     saveGeometry());
  settings.setValue(FOLDER,       m_baseDir->text());
  settings.setValue(DESTINATION,  m_destinationDir->text());
  settings.setValue(COPYSIZE,     m_amount->currentIndex());
  settings.setValue(COPYUNITS,    m_units->currentIndex());
  settings.setValue(USEWINAMP,    m_winamp->isChecked());
  settings.setValue(USESMPLAYER,  m_smplayer->isChecked());
  settings.setValue(SUBTITLESIZE, static_cast<double>(m_subtitleSizeSlider->value()/10.));
  settings.setValue(WINAMP_LOC,   m_winampPath);
  settings.setValue(SMPLAYER_LOC, m_smplayerPath);
  settings.setValue(CASTNOW_LOC,  m_castnowPath);
  settings.sync();
}


//-----------------------------------------------------------------------------
void NowPlay::callWinamp()
{
  if(!Utils::checkIfValidWinAmpLocation(m_winampPath)) return;

  auto handler = WinAmp::getWinAmpHandle(m_winampPath);

  if(!handler)
  {
    showErrorMessage(tr("Unable to launch or contact WinAmp"));
    return;
  }

  WinAmp::deletePlaylist(handler);

  auto isPlaylist = [this](const Utils::FileInformation &f){ return Utils::isPlaylistFile(f.first); };
  auto it = std::find_if(m_files.cbegin(), m_files.cend(), isPlaylist);
  if(it != m_files.cend())
  {
    WinAmp::addFile(handler, (*it).first.wstring());
  }
  else
  {
    auto checkAndAdd = [&](const Utils::FileInformation &f)
    {
      if(Utils::isAudioFile(f.first))
      {
        WinAmp::addFile(handler, f.first.wstring());
        return true;
      }

      return false;
    };
    auto count = std::count_if(m_files.cbegin(), m_files.cend(), checkAndAdd);

    if(count == 0)
    {
      const auto message = tr("No files found in directory: ") + QString::fromStdWString(m_files.front().first.parent_path().wstring());
      showErrorMessage(message);
      return;
    }
  }

  m_files.clear();

  WinAmp::startPlay(handler);
}

//-----------------------------------------------------------------------------
void NowPlay::playVideos()
{
  if(!Utils::checkIfValidSMPlayerLocation(m_smplayerPath)) return;

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

  m_process.start(m_smplayerPath, arguments);
}

//-----------------------------------------------------------------------------
void NowPlay::castFile()
{
  if(m_process.state() == QProcess::ProcessState::Running)
  {
    m_process.kill();
    m_files.clear();
    m_tabWidget->setEnabled(true);

    m_progress->setValue(0);
    m_taskBarButton->progress()->setValue(0);
    m_taskBarButton->progress()->setVisible(false);
    m_progress->setEnabled(false);
    m_play->setText("Now Play!");
    return;
  }

  if(!Utils::checkIfValidCastnowLocation(m_castnowPath)) return;

  auto isValidFile = [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); };
  auto file = std::find_if(m_files.begin(), m_files.end(), isValidFile);
  if(file != m_files.end())
  {
    const auto filename = (*file).first;
    m_files.erase(file);

    m_progress->setValue(m_progress->value() + 1);
    if(!m_taskBarButton->progress()->isVisible()) m_taskBarButton->progress()->setVisible(true);
    m_taskBarButton->progress()->setValue(m_progress->value());

    log(QString::fromStdWString(filename.filename().wstring()));

    QStringList arguments;
    arguments << QDir::fromNativeSeparators(QString::fromStdWString(filename.wstring()));
    if(Utils::isVideoFile(filename.string()))
    {
      arguments << "--subtitle-scale";
      arguments << m_subtitleSizeLabel->text();
    }

    m_process.start(m_castnowPath, arguments, QProcess::Unbuffered|QProcess::ReadWrite);
    m_process.waitForStarted();

    m_icon->showMessage(QString::fromStdWString(filename.parent_path().filename().c_str()), QString::fromStdWString(filename.c_str()), QIcon(":/NowPlay/buttons.svg"));
  }
  else
  {
    m_files.clear();
    m_tabWidget->setEnabled(true);

    m_progress->setValue(0);
    m_progress->setEnabled(false);
    m_taskBarButton->progress()->setValue(0);
    m_taskBarButton->progress()->setVisible(false);
    m_play->setText("Now Play!");
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
  if(m_process.state() == QProcess::ProcessState::Running)
  {
    m_process.putChar('s');
    m_process.putChar('q');
    m_process.kill();
    m_files.clear();
    m_tabWidget->setEnabled(true);

    m_progress->setValue(0);
    m_progress->setEnabled(false);
    m_taskBarButton->progress()->setValue(0);
    m_play->setText("Now Play!");
    m_next->setEnabled(false);
    m_icon->contextMenu()->actions().at(1)->setText("Now Play!");
    m_icon->contextMenu()->actions().at(2)->setEnabled(false);
    return;
  }

  const bool isCopyMode = m_tabWidget->currentIndex() == 1;

  boost::filesystem::path directory = m_baseDir->text().toStdWString();
  auto validPaths = Utils::getSubdirectories(directory, isCopyMode);

  // Copy mode
  if(isCopyMode)
  {
    const auto destination = m_destinationDir->text().toStdWString();
    bool ok = false;
    auto size = m_amount->currentText().toInt(&ok, 10);

    if(!ok)
    {
      showErrorMessage(tr("Invalid size option value."));
      return;
    }

    if (destination.empty() || !filesystem::exists(destination))
    {
      showErrorMessage(tr("Invalid destination directory."));
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

    unsigned long long accumulator = 0L;
    auto printInfo = [&accumulator, this](const Utils::FileInformation &f)
    {
      QString message = tr("Selected: ") + QString::fromStdWString(f.first.filename().wstring()) + " (" + QString::number(f.second) + ")";
      log(message);

      accumulator += f.second;
    };
    std::for_each(selectedDirs.cbegin(), selectedDirs.cend(), printInfo);

    message = tr("Total bytes ") + QString::number(accumulator) + " in " + QString::number(selectedDirs.size()) + " directories, remaining to limit " + QString::number(size - accumulator) + " bytes.";
    log(message);

    if (!selectedDirs.empty())
    {
      if(destination.empty() || !filesystem::exists(destination) || !filesystem::is_directory(destination))
      {
        showErrorMessage(tr("No destination directory to copy to."));
        return;
      }

      log(tr("Copying directories..."));
      m_play->setEnabled(false);

      QApplication::setOverrideCursor(Qt::WaitCursor);

      int i = 0;
      m_progress->setEnabled(true);
      for(auto dir: selectedDirs)
      {
        m_progress->setValue((100*i)/selectedDirs.size());
        m_taskBarButton->progress()->setValue(m_progress->value());

        if(!Utils::copyDirectory(dir.first.string(), destination))
        {
          const QString message = QString("Error while copying files of directory: ") + QString::fromStdWString(dir.first.wstring());
          QApplication::restoreOverrideCursor();
          m_play->setEnabled(true);
          showErrorMessage(message, tr("Copy error"));
          return;
        }

        ++i;
      }

      QApplication::restoreOverrideCursor();

      m_progress->setValue(100);
      m_progress->setEnabled(false);
      m_taskBarButton->progress()->setValue(100);

      m_play->setEnabled(true);
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
    QString message = tr("<b>") + QString::fromStdWString(directory.wstring()) + tr("</b> has ") + QString::number(validPaths.size()) + tr(" directories.");
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
    QString message = QString("Base directory: <b>") + QString::fromStdWString(directory.wstring()) + tr("</b>");
    log(message);
  }

  m_files = Utils::getPlayableFiles(directory);

  if(!m_files.empty())
  {
    if(m_winamp->isChecked())
    {
      callWinamp();
    }
    else
    {
      if(m_castnow->isChecked())
      {
        m_play->setText("Stop");
        m_next->setEnabled(true);
        m_icon->contextMenu()->actions().at(1)->setText("Stop");
        m_icon->contextMenu()->actions().at(2)->setEnabled(true);

        m_progress->setEnabled(true);
        const auto count = std::count_if(m_files.cbegin(), m_files.cend(), [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); });
        m_progress->setMinimum(0);
        m_progress->setMaximum(count);
        m_taskBarButton->progress()->setMinimum(0);
        m_taskBarButton->progress()->setMinimum(count);
        m_progress->setValue(0);

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

  if(!newDirectory.isEmpty()) widget->setText(newDirectory);

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
  if(m_process.state() == QProcess::ProcessState::Running)
  {
    std::cout << "pressed " << e->key() << " or " << e->text().toLocal8Bit().constData() << " l " << e->text().length() << std::endl;
    e->accept();

    m_process.write(e->text().toLocal8Bit().constData(), e->text().length());
    std::cout << m_process.bytesToWrite() << " " <<  m_process.bytesAvailable();
    if(!m_process.waitForBytesWritten()) std::cout << "write wait fail" << std::endl;
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
}

//-----------------------------------------------------------------------------
void NowPlay::checkApplications()
{
  const auto validWinamp   = Utils::checkIfValidWinAmpLocation(m_winampPath);
  const auto validSMPlayer = Utils::checkIfValidSMPlayerLocation(m_smplayerPath);
  const auto validCastnow  = Utils::checkIfValidCastnowLocation(m_castnowPath);

  m_winamp->setEnabled(validWinamp);
  m_smplayer->setEnabled(validSMPlayer);
  m_castnow->setEnabled(validCastnow);

  const auto isValid = validWinamp || validSMPlayer || validCastnow;
  m_play->setEnabled(isValid);
}

//-----------------------------------------------------------------------------
bool NowPlay::event(QEvent *event)
{
  if(event->type() == QEvent::KeyPress)
  {
    auto ke = static_cast<QKeyEvent *>(event);
    keyPressEvent(ke);

    std::cout << m_process.readAllStandardError().constData() << std::endl;
    return true;
  }

  return QDialog::event(event);
}

//-----------------------------------------------------------------------------
void NowPlay::onOuttputAvailable()
{
  const auto data = m_process.readAll().toStdString();
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
    m_log->document()->undo();
    m_log->append("<font color=\"red\">");
    m_log->document()->redo();
    m_log->append("</font>");

    m_process.kill();
    m_process.waitForFinished(-1);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::onSubtitleSizeChanged(int value)
{
  m_subtitleSizeLabel->setText(QString::number(static_cast<double>(value/10.), 'e', 2));
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
  SettingsDialog dialog(m_winampPath, m_smplayerPath, m_castnowPath, this);
  if(QDialog::Accepted == dialog.exec())
  {
    m_winampPath = dialog.getWinampLocation();
    m_smplayerPath = dialog.getSmplayerLocation();
    m_castnowPath = dialog.getCastnowLocation();

    checkApplications();
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
          this, SIGNAL(close()));

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
void NowPlay::onRestoreActionActivated()
{
  m_icon->hide();
  showNormal();
}

//-----------------------------------------------------------------
void NowPlay::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  m_taskBarButton = new QWinTaskbarButton(this);
  m_taskBarButton->setWindow(this->windowHandle());
  m_taskBarButton->progress()->setVisible(false);
  m_taskBarButton->progress()->setValue(0);
}
