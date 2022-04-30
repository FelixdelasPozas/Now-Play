/*
 File: Utils.cpp
 Created on: 21 ene. 2020
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

// Project
#include <Utils.h>

// C++
#include <numeric>
#include <random>
#include <chrono>
#include <algorithm>

// Qt
#include <QFileInfo>

//-----------------------------------------------------------------------------
bool Utils::isAudioFile(const std::filesystem::path &path)
{
  auto extension = path.extension().string();
  toLower(extension);

  return std::filesystem::is_regular_file(path) && (extension.compare(".mp3") == 0 || extension.compare(".m4a") == 0);
}

//-----------------------------------------------------------------------------
bool Utils::isPlaylistFile(const std::filesystem::path &path)
{
  auto extension = path.extension().string();
  toLower(extension);

  return std::filesystem::is_regular_file(path) && (extension.compare(".m3u") == 0 || extension.compare(".m3u8") == 0);
}

//-----------------------------------------------------------------------------
bool Utils::isVideoFile(const std::filesystem::path &path)
{
  auto extension = path.extension().string();
  toLower(extension);

  return std::filesystem::is_regular_file(path) &&
         (extension.compare(".mp4") == 0 || extension.compare(".mkv") == 0 || extension.compare(".webm") == 0);
}

//-----------------------------------------------------------------------------
std::vector<Utils::FileInformation> Utils::getPlayableFiles(const std::filesystem::path &directory)
{
  std::vector<FileInformation> files;

  if(!directory.empty() && std::filesystem::is_directory(directory))
  {
    for(const auto &it: std::filesystem::recursive_directory_iterator{directory})
    {
      const auto name = it.path();
      if(name.filename().string().compare(".") == 0 || name.filename().string().compare("..") == 0) continue;

      if(Utils::isAudioFile(name) || Utils::isVideoFile(name) || Utils::isPlaylistFile(name))
      {
        files.emplace_back(name, std::filesystem::file_size(name));
      }
    }
  }

  std::sort(files.begin(), files.end(), lessThan);

  return files;
}

//-----------------------------------------------------------------------------
std::vector<Utils::FileInformation> Utils::getSubdirectories(const std::filesystem::path &directory, bool readSize)
{
  std::vector<FileInformation> directories;

  if(!directory.empty() && std::filesystem::is_directory(directory))
  {
    for(const auto &it: std::filesystem::recursive_directory_iterator{directory})
    {
      const auto name = it.path();
      if(name.filename().string().compare(".") == 0 || name.filename().string().compare("..") == 0) continue;

      if(std::filesystem::is_directory(it))
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

  std::sort(directories.begin(), directories.end(), lessThan);

  return directories;
}

//-----------------------------------------------------------------------------
std::vector<Utils::FileInformation> Utils::getCopyDirectories(std::vector<Utils::FileInformation> &dirs, const unsigned long long size)
{
  std::vector<FileInformation> selectedDirs;

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

  std::sort(selectedDirs.begin(), selectedDirs.end(), Utils::lessThan);

  return selectedDirs;
}

//-----------------------------------------------------------------------------
bool Utils::copyDirectory(const std::filesystem::path &from, const std::filesystem::path &to)
{
  const wchar_t SEPARATOR = '/';

  std::error_code error;

  const auto newFolder = to.wstring() + SEPARATOR + from.filename().wstring();
  std::filesystem::create_directory(newFolder);

  const auto files = getPlayableFiles(from);

  for(auto file: files)
  {
    auto fullPath = newFolder + SEPARATOR + file.first.filename().wstring();
    std::filesystem::copy_file(file.first, fullPath, error);

    if (error.value() != 0)
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Utils::lessThan(const FileInformation &lhs, const FileInformation &rhs)
{
  return lhs.first < rhs.first;
}

//-----------------------------------------------------------------------------
bool Utils::checkIfValidMusicPlayerLocation(const QString &location)
{
  if(!location.isEmpty())
  {
    QFileInfo file(location);
    return file.exists();
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Utils::checkIfValidVideoPlayerLocation(const QString &location)
{
  if(!location.isEmpty())
  {
    QFileInfo file(location);
    return file.exists();
  }

  return false;
}

//-----------------------------------------------------------------------------
bool Utils::checkIfValidCastnowLocation(const QString &location)
{
  if(!location.isEmpty())
  {
    QFileInfo file(location);
    return file.exists() && (location.endsWith("castnow.cmd", Qt::CaseInsensitive) || location.endsWith("castnow", Qt::CaseInsensitive));
  }

  return false;
}

//-----------------------------------------------------------------------------
void Utils::toLower(std::string &s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
}
