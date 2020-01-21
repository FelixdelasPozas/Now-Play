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

// Qt
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

// Boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;

const QString GEOMETRY    = "Geometry";
const QString FOLDER      = "Folder";
const QString COPYSIZE    = "Copy Size";
const QString COPYUNITS   = "Copy Units";
const QString DESTINATION = "Destination Folder";
const QString USEWINAMP   = "Play In Winamp";

//-----------------------------------------------------------------------------
NowPlay::NowPlay()
: QDialog(nullptr)
{
  setupUi(this);

  loadSettings();

  connectSignals();

  m_tabWidget->setCurrentIndex(0);
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

  const auto amount = settings.value(COPYSIZE, 0).toInt();

  m_amount->setCurrentIndex(amount);

  const auto units = settings.value(COPYUNITS, 0).toInt();

  m_units->setCurrentIndex(units);

  const auto useWinamp = settings.value(USEWINAMP, false).toBool();

  m_winamp->setChecked(useWinamp);
  m_castnow->setChecked(!useWinamp);
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
  settings.sync();
}

//-----------------------------------------------------------------------------
bool NowPlay::isAudioFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && extension.compare(".mp3") == 0;
}

//-----------------------------------------------------------------------------
bool NowPlay::isPlaylistFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && (extension.compare(".m3u") == 0 || extension.compare(".m3u8") == 0);
}

//-----------------------------------------------------------------------------
bool NowPlay::isVideoFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && (extension.compare(".mp4") == 0 || extension.compare(".mkv") == 0);
}

//-----------------------------------------------------------------------------
std::vector<NowPlay::FileInformation> NowPlay::getPlayableFiles(const std::string &directory)
{
  std::vector<FileInformation> files;

  if(!directory.empty() && filesystem::is_directory(directory.c_str()))
  {
    for(filesystem::directory_entry &it: filesystem::directory_iterator(directory.c_str()))
    {
      const auto name = it.path();
      if(name.filename_is_dot() || name.filename_is_dot_dot()) continue;

      if(isAudioFile(name) || isVideoFile(name) || isPlaylistFile(name))
      {
        files.emplace_back(name, filesystem::file_size(name));
      }
    }
  }

  return files;
}

//-----------------------------------------------------------------------------
std::vector<NowPlay::FileInformation> NowPlay::getSubdirectories(const std::string &directory, bool readSize)
{
  std::vector<FileInformation> directories;

  if(!directory.empty() && filesystem::is_directory(directory.c_str()))
  {
    for(filesystem::directory_entry &it: filesystem::directory_iterator(directory.c_str()))
    {
      const auto name = it.path();
      if(name.filename_is_dot() || name.filename_is_dot_dot()) continue;

      if(filesystem::is_directory(it))
      {
        unsigned long long size = 0L;
        if(readSize)
        {
          auto files = getPlayableFiles(it.path().string());

          auto addOp = [](const unsigned long long &s, const FileInformation &f)
          {
            return s + f.second;
          };
          size = std::accumulate(files.cbegin(), files.cend(), 0L, addOp);
        }

        directories.emplace_back(it.path(), size);
      }
    }
  }

  return directories;
}

//-----------------------------------------------------------------------------
void NowPlay::callWinamp(std::vector<FileInformation> &files)
{
  auto handler = WinAmp::getWinAmpHandle();

  if(!handler)
  {
    showErrorMessage(tr("Unable to launch or contact WinAmp"));
    return;
  }

  WinAmp::deletePlaylist(handler);

  auto isPlaylist = [this](const FileInformation &f){ return isPlaylistFile(f.first); };
  auto it = std::find_if(files.cbegin(), files.cend(), isPlaylist);
  if(it != files.cend())
  {
    WinAmp::addFile(handler, (*it).first.string());
  }
  else
  {
    auto checkAndAdd = [&](const FileInformation &f)
    {
      if(isAudioFile(f.first))
      {
        WinAmp::addFile(handler, f.first.string());
        return true;
      }

      return false;
    };
    auto count = std::count_if(files.cbegin(), files.cend(), checkAndAdd);

    if(count == 0)
    {
      const auto message = tr("No files found in directory ") + QString::fromStdString(files.front().first.parent_path().string());
      showErrorMessage(message);
      return;
    }
  }

  WinAmp::startPlay(handler);
}

//-----------------------------------------------------------------------------
void NowPlay::castFiles(std::vector<FileInformation> &files)
{
  std::sort(files.begin(), files.end(), NowPlay::lessThan);

  int i = 0;
  auto isPlaylist = [&](const FileInformation &f){ return isPlaylistFile(f.first); };
  const int total = files.size() - std::count_if(files.cbegin(), files.cend(), isPlaylist);

  auto playFile = [&i, total, this](const FileInformation &f)
  {
    if(isPlaylistFile(f.first)) return;

    const auto message = QString::fromStdString(f.first.filename().string()) + " (" + QString::number(total) + "/" + QString::number(i) + ")";
    log(message);

    const std::string subtitleParams = isVideoFile(f.first) ? " --subtitle-scale 1.3 ":"";

    const auto command = std::string("echo off & castnow \"") + f.first.string() + "\"" +  subtitleParams + " --quiet";
    std::system(command.c_str());
  };
  std::for_each(files.cbegin(), files.cend(), playFile);
}

//-----------------------------------------------------------------------------
std::vector<NowPlay::FileInformation> NowPlay::getCopyDirectories(std::vector<FileInformation> &dirs, const unsigned long long size)
{
  std::vector<FileInformation> selectedDirs;

  QString message = tr("Selecting from base for ") + QString::number(size) + " bytes...";
  log(message);

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);

  bool finished = false;
  unsigned long long remaining = size;

  while(!finished)
  {
    std::uniform_int_distribution<int> distribution(1, dirs.size());
    const int roll = distribution(generator);
    const auto selectedPath = dirs.at(roll-1);

    if(selectedPath.second == 0)
    {
      dirs.erase(dirs.begin() + roll-1);
      continue;
    }

    if(dirs.empty() || selectedPath.second > remaining)
    {
      finished = true;

      auto computeMaximumFit = [&remaining, &selectedDirs](const FileInformation &f)
      {
        if(f.second < remaining)
        {
          remaining -= f.second;
          selectedDirs.push_back(f);
        }
      };
      std::for_each(dirs.begin(), dirs.end(), computeMaximumFit);
    }
    else
    {
      selectedDirs.push_back(selectedPath);
      remaining -= selectedPath.second;
      dirs.erase(dirs.begin() + roll-1);
    }
  }

  std::sort(selectedDirs.begin(), selectedDirs.end(), NowPlay::lessThan);

  unsigned long long accumulator = 0L;
  auto printInfo = [&accumulator, this](const FileInformation &f)
  {
    QString message = tr("Selected: ") + QString::fromStdString(f.first.filename().string()) + " (" + QString::number(f.second) + ")";
    log(message);

    accumulator += f.second;
  };
  std::for_each(selectedDirs.cbegin(), selectedDirs.cend(), printInfo);

  message = tr("Total bytes ") + QString::number(accumulator) + " in " + QString::number(selectedDirs.size()) + " directories, remaining to limit " + QString::number(remaining) + " bytes.";
  log(message);

  return selectedDirs;
}

//-----------------------------------------------------------------------------
void NowPlay::copyDirectories(const std::vector<FileInformation> &dirs, const std::string &to)
{
  if(dirs.empty())
  {
    showErrorMessage(tr("No directories to copy."));
    return;
  }

  if(to.empty() || !filesystem::exists(to) || !filesystem::is_directory(to))
  {
    showErrorMessage(tr("No destination directory to copy to."));
    return;
  }

  filesystem::directory_entry toEntry(to);

  system::error_code error;

  int i = 0;
  const float progressUnit = 100.0/dirs.size();



  auto copyDirectory = [&i, &toEntry, &error, &progressUnit, this](const FileInformation &dir)
  {
    const auto progress = (i++)*progressUnit;

    const auto newFolder = toEntry.path().string() + SEPARATOR + dir.first.filename().string();
    filesystem::create_directory(newFolder);

    const auto files = getPlayableFiles(dir.first.string());
    const auto currentUnit = progressUnit/files.size();

    int j = 0;
    for(auto file: files)
    {
      QString copyMessage = QString("\rCopying files... ") + QString::number(static_cast<int>(progress + (j++)*currentUnit)) + "%";
      log(copyMessage);

      auto fullPath = newFolder + SEPARATOR + file.first.filename().string();
      filesystem::copy_file(file.first, fullPath, error);

      if(error.value() != 0)
      {
        const auto message = QString(tr("Error when copying file: ")) + QString::fromStdString(file.first.string());
        const auto details = QString(tr("Details: ")) + QString::fromStdString(error.message());
        const auto title   = QString(tr("Error while copying files"));
        showErrorMessage(message, title, details);
        return;
      }
    }
  };
  std::for_each(dirs.cbegin(), dirs.cend(), copyDirectory);

  log(tr("Copying done!"));
}

//-----------------------------------------------------------------------------
void NowPlay::connectSignals()
{
  connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
  connect(m_browseBase, SIGNAL(pressed()), this, SLOT(browseDir()));
  connect(m_browseDestination, SIGNAL(pressed()), this, SLOT(browseDir()));
  connect(m_about, SIGNAL(pressed()), this, SLOT(onAboutButtonClicked()));
  connect(m_play, SIGNAL(pressed()), this, SLOT(onPlayButtonClicked()));
  connect(m_exit, SIGNAL(pressed()), this, SLOT(close()));
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
  const bool isCopyMode = m_tabWidget->currentIndex() == 1;

  boost::filesystem::path directory = m_baseDir->text().toStdString();
  auto validPaths = getSubdirectories(directory.string(), isCopyMode);

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

    auto selectedDirs = getCopyDirectories(validPaths, size);

    if (!selectedDirs.empty())
    {
      copyDirectories(selectedDirs, destination);
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

  auto files = getPlayableFiles(directory.string());

  if(!files.empty())
  {
    if(m_winamp->isChecked())
    {
      callWinamp(files);
    }
    else
    {
      castFiles(files);
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
  // TODO
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
  m_log->appendPlainText("\n");
}
