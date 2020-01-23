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

// Qt
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

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
const QString SUBTITLESIZE = "Subtitle Size";

const unsigned long long MEGABYTE = 1024*1024;

const QString CASTNOW_LOCATION = QString::fromWCharArray(L"C:/Users/Felix/AppData/Roaming/npm/castnow.cmd");

//-----------------------------------------------------------------------------
NowPlay::NowPlay()
: QDialog  {nullptr}
, m_process{this}
{
  setupUi(this);

  loadSettings();

  updateGUI();

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

  m_winamp->setChecked(useWinamp);
  m_castnow->setChecked(!useWinamp);

  const auto subtitleSize = settings.value(SUBTITLESIZE, 1.3).toDouble();

  m_subtitleSizeSlider->setValue(subtitleSize * 10);
  onSubtitleSizeChanged(subtitleSize * 10);
}

//-----------------------------------------------------------------------------
void NowPlay::saveSettings()
{
  QSettings settings("Felix de las Pozas Alvarez", "NowPlay");

  settings.setValue(GEOMETRY, saveGeometry());
  settings.setValue(FOLDER, m_baseDir->text());
  settings.setValue(DESTINATION, m_destinationDir->text());
  settings.setValue(COPYSIZE, m_amount->currentIndex());
  settings.setValue(COPYUNITS, m_units->currentIndex());
  settings.setValue(USEWINAMP, m_winamp->isChecked());
  settings.setValue(SUBTITLESIZE, static_cast<double>(m_subtitleSizeSlider->value()/10.));
  settings.sync();
}


//-----------------------------------------------------------------------------
void NowPlay::callWinamp()
{
  auto handler = WinAmp::getWinAmpHandle();

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
    WinAmp::addFile(handler, (*it).first.string());
  }
  else
  {
    auto checkAndAdd = [&](const Utils::FileInformation &f)
    {
      if(Utils::isAudioFile(f.first))
      {
        WinAmp::addFile(handler, f.first.string());
        return true;
      }

      return false;
    };
    auto count = std::count_if(m_files.cbegin(), m_files.cend(), checkAndAdd);

    if(count == 0)
    {
      const auto message = tr("No files found in directory: ") + QString::fromStdString(m_files.front().first.parent_path().string());
      showErrorMessage(message);
      return;
    }
  }

  m_files.clear();

  WinAmp::startPlay(handler);
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
    m_progress->setEnabled(false);
    m_play->setText("Now Play!");
    return;
  }

  auto isValidFile = [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); };
  auto file = std::find_if(m_files.begin(), m_files.end(), isValidFile);
  if(file != m_files.end())
  {
    const auto filename = (*file).first;
    m_files.erase(file);

    m_progress->setValue(m_progress->value() + 1);

    log(QString::fromStdWString(filename.filename().wstring()));

    QStringList arguments;
    arguments << QDir::fromNativeSeparators(QString::fromStdWString(filename.wstring()));
    if(Utils::isVideoFile(filename.string()))
    {
      arguments << "--subtitle-scale";
      arguments << m_subtitleSizeLabel->text();
    }

    m_process.start(CASTNOW_LOCATION, arguments, QProcess::Unbuffered|QProcess::ReadWrite);
    m_process.waitForStarted();
  }
  else
  {
    m_files.clear();
    m_tabWidget->setEnabled(true);

    m_progress->setValue(0);
    m_progress->setEnabled(false);
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
  connect(m_exit,               SIGNAL(pressed()),           this, SLOT(close()));
  connect(m_subtitleSizeSlider, SIGNAL(valueChanged(int)),   this, SLOT(onSubtitleSizeChanged(int)));

  connect(&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onOuttputAvailable()));
  connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(castFile()));
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
    m_play->setText("Now Play!");
    return;
  }

  const bool isCopyMode = m_tabWidget->currentIndex() == 1;

  boost::filesystem::path directory = m_baseDir->text().toStdString();
  auto validPaths = Utils::getSubdirectories(directory.string(), isCopyMode);

  // Copy mode
  if(isCopyMode)
  {
    const auto destination = m_destinationDir->text().toStdString();
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
      QString message = tr("Selected: ") + QString::fromStdString(f.first.filename().string()) + " (" + QString::number(f.second) + ")";
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

        if(!Utils::copyDirectory(dir.first.string(), destination))
        {
          const QString message = QString("Error while copying files of directory: ") + QString::fromStdString(dir.first.string());
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

  // Cast mode.
  m_play->setText("Stop");

  if(!validPaths.empty())
  {
    QString message = QString::fromStdString(directory.string()) + " has " + QString::number(validPaths.size()) + " directories.";
    log(message);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);

    std::uniform_int_distribution<int> distribution(1, validPaths.size());
    int roll = distribution(generator);

    directory = validPaths.at(roll-1).first;

    message = QString("Selected: ") + QString::fromStdString(directory.filename().string());
    log(message);
  }
  else
  {
    QString message = QString("Base directory: ") + QString::fromStdString(directory.string());
    log(message);
  }

  m_files = Utils::getPlayableFiles(directory.string());

  if(!m_files.empty())
  {
    if(m_winamp->isChecked())
    {
      callWinamp();
    }
    else
    {
      m_progress->setEnabled(true);
      m_progress->setMinimum(0);
      const auto count = std::count_if(m_files.cbegin(), m_files.cend(), [](const Utils::FileInformation &f){ return Utils::isAudioFile(f.first) || Utils::isVideoFile(f.first); });
      m_progress->setMaximum(count);
      m_progress->setValue(0);

      m_tabWidget->setEnabled(false);

      castFile();
    }
  }
  else
  {
    const auto message = QString("No music files found in directory: ") + QString::fromStdString(directory.string());
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
  m_log->appendPlainText(message);
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
  const auto pos = data.find("Idle...");
  if(std::string::npos != pos)
  {
    m_process.kill();
    m_process.waitForFinished(-1);
  }
}

//-----------------------------------------------------------------------------
void NowPlay::onSubtitleSizeChanged(int value)
{
  m_subtitleSizeLabel->setText(QString::number(static_cast<double>(value/10.), 'e', 2));
}
