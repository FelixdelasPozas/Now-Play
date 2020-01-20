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
#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <chrono>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <unistd.h>

// Project
#include "termcolor.h"
#include "version.h"
#include "winampapi.h"

// Boost
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

const char SEPARATOR = '/';

const unsigned long long MEGABYTE = 1024*1024;

using namespace boost;

using FileInformation = std::pair<filesystem::path, uint64_t>;

//-----------------------------------------------------------------------------
bool lessThan(const FileInformation &lhs, const FileInformation &rhs)
{
  return lhs.first < rhs.first;
}

//-----------------------------------------------------------------------------
bool isAudioFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && extension.compare(".mp3") == 0;
}

//-----------------------------------------------------------------------------
bool isPlaylistFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && (extension.compare(".m3u") == 0 || extension.compare(".m3u8") == 0);
}

//-----------------------------------------------------------------------------
bool isVideoFile(const filesystem::path &path)
{
  auto extension = path.extension().string();
  boost::algorithm::to_lower(extension);

  return filesystem::is_regular_file(path) && (extension.compare(".mp4") == 0 || extension.compare(".mkv") == 0);
}

//-----------------------------------------------------------------------------
void banner()
{
  std::cout << termcolor::reset;
  std::cout << "NowPlay v1.5 (build " << BUILD_NUMBER << ", " << __DATE__ << " " << __TIME__ << ")" << std::endl;
}

//-----------------------------------------------------------------------------
std::vector<FileInformation> getPlayableFiles(const std::string &directory)
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
std::vector<FileInformation> getSubdirectories(const std::string &directory, bool readSize = false)
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
void callWinamp(std::vector<FileInformation> &files)
{
  auto handler = WinAmp::getWinAmpHandle();

  if(!handler)
  {
    std::cout << "ERROR: Couldn't launch Winamp." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  WinAmp::deletePlaylist(handler);

  auto isPlaylist = [](const FileInformation &f){ return isPlaylistFile(f.first); };
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
      std::cout << "ERROR: No files found in directory: " << files.front().first.parent_path().string() << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  WinAmp::startPlay(handler);
}

//-----------------------------------------------------------------------------
void castFiles(std::vector<FileInformation> &files)
{
  std::sort(files.begin(), files.end(), lessThan);

  int i = 0;
  auto isPlaylist = [&](const FileInformation &f){ return isPlaylistFile(f.first); };
  const int total = files.size() - std::count_if(files.cbegin(), files.cend(), isPlaylist);

  auto playFile = [&i, total](const FileInformation &f)
  {
    if(isPlaylistFile(f.first)) return;

    std::cout << termcolor::grey << termcolor::on_white << f.first.filename().string() << termcolor::reset << " (" << ++i << "/" << total << ")";

    const std::string subtitleParams = isVideoFile(f.first) ? " --subtitle-scale 1.3 ":"";

    const auto command = std::string("echo off & castnow \"") + f.first.string() + "\"" +  subtitleParams + " --quiet";
    std::system(command.c_str());

    std::cout << "\r" << f.first.filename().string() << "      " << std::endl;
  };
  std::for_each(files.cbegin(), files.cend(), playFile);
}

//-----------------------------------------------------------------------------
std::vector<FileInformation> getCopyDirectories(std::vector<FileInformation> &dirs, const unsigned long long size)
{
  std::vector<FileInformation> selectedDirs;

  std::cout << "Selecting from base for " << size << " bytes..." << std::endl;

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

  std::sort(selectedDirs.begin(), selectedDirs.end(), lessThan);

  unsigned long long accumulator = 0L;
  auto printInfo = [&accumulator](const FileInformation &f)
  {
    std::cout << "Selected: " << f.first.filename().string() << " (" << f.second << ")" << std::endl;
    accumulator += f.second;
  };
  std::for_each(selectedDirs.cbegin(), selectedDirs.cend(), printInfo);
  std::cout << "Total bytes " << accumulator << " in " << selectedDirs.size() << " directories, remaining to limit " << remaining << " bytes." << std::endl;

  return selectedDirs;
}

//-----------------------------------------------------------------------------
void copyDirectories(const std::vector<FileInformation> &dirs, std::string &to)
{
  if(dirs.empty())
  {
    std::cout << "ERROR: No directories to copy." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if(to.empty() || !filesystem::exists(to) || !filesystem::is_directory(to))
  {
    std::cout << "ERROR: No destination directory to copy to." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  filesystem::directory_entry toEntry(to);

  system::error_code error;

  int i = 0;
  const float progressUnit = 100.0/dirs.size();

  auto copyDirectory = [&i, &toEntry, &error, &progressUnit](const FileInformation &dir)
  {
    const auto progress = (i++)*progressUnit;

    const auto newFolder = toEntry.path().string() + SEPARATOR + dir.first.filename().string();
    filesystem::create_directory(newFolder);

    const auto files = getPlayableFiles(dir.first.string());
    const auto currentUnit = progressUnit/files.size();

    int j = 0;
    for(auto file: files)
    {
      std::cout << "\r" << "Copying files... " << termcolor::grey << termcolor::on_white << static_cast<int>(progress + (j++)*currentUnit) << "%" << termcolor::reset;

      auto fullPath = newFolder + SEPARATOR + file.first.filename().string();
      filesystem::copy_file(file.first, fullPath, error);

      if(error.value() != 0)
      {
        std::cout << "ERROR: Cannot continue file copy. " << std::endl;
        std::cout << "ERROR: Error when copying file: " << file.first << std::endl;
        std::cout << "ERROR: Message: " << error.message() << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }
  };
  std::for_each(dirs.cbegin(), dirs.cend(), copyDirectory);

  std::cout << "\r" << "Copying done!        " << std::endl;
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  banner();

  unsigned long long size = 0L;
  bool playInWinamp = false;
  std::string destination;

  program_options::options_description desc("Options");
  desc.add_options()
        ("help,h", "Print application help message.")
        ("winamp,w", "Play in WinAmp.")
        ("size,s", program_options::value<unsigned long long>(&size), "Total size of directories to copy to destination in megabytes.")
        ("destination,d", program_options::value<std::string>(&destination), "Destination of the selected archives.");

  program_options::variables_map vm;

  try
  {
    program_options::store(program_options::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }

    playInWinamp = vm.count("winamp");

    program_options::notify(vm);
  }
  catch (program_options::error &e)
  {
    std::cout << desc << std::endl;
    std::cout << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  auto isCopyMode = (size != 0L) || !destination.empty();

  if(playInWinamp && isCopyMode)
  {
    std::cout << desc << std::endl;
    std::cout << "ERROR: Invalid combination of parameters. Can't copy and play at the same time." << std::endl;
    return EXIT_FAILURE;
  }

  auto directory = filesystem::current_path();

  std::cout << "Base directory: " << directory.string();

  auto validPaths = getSubdirectories(directory.string(), isCopyMode);

  // Copy mode
  if(isCopyMode)
  {
    std::cout << std::endl;

    if(size == 0L)
    {
      std::cout << "ERROR: Invalid size option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    if(destination.empty() || !filesystem::exists(destination))
    {
      std::cout << "ERROR: Invalid destination option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    if(validPaths.empty())
    {
      std::cout << "ERROR: No subdirectories to select from." << std::endl;
      return EXIT_FAILURE;
    }

    auto selectedDirs = getCopyDirectories(validPaths, size*MEGABYTE);

    if(!selectedDirs.empty())
    {
      copyDirectories(selectedDirs, destination);
    }
    else
    {
      std::cout << "ERROR: Unable to select directories for the given size: " << size << " Mb." << std::endl;
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  // Cast mode.
  if(!validPaths.empty())
  {
    std::cout << " (" << validPaths.size() << " directories)" << std::endl;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);

    std::uniform_int_distribution<int> distribution(1, validPaths.size());
    int roll = distribution(generator);

    directory = validPaths.at(roll-1).first;

    std::cout << "Selected: " << termcolor::grey << termcolor::on_white << directory.filename().string() << termcolor::reset << std::endl;
  }
  else
  {
    std::cout << "\r" << "Base directory: " << termcolor::grey << termcolor::on_white << directory.string() << termcolor::reset << std::endl;
  }

  auto files = getPlayableFiles(directory.string());

  if(!files.empty())
  {
    if(playInWinamp)
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
    std::cout << "No music files found in directory: " << directory.string() << std::endl;
  }

  return EXIT_SUCCESS;
}
