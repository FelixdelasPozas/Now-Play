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

// Project
#include "termcolor.h"
#include "version.h"

// Boost
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

const char SEPARATOR = '/';

const unsigned long long MEGABYTE = 1024*1024;

using namespace boost;

using FileInformation = std::pair<filesystem::path, uint64_t>;

//-----------------------------------------------------------------------------
void banner()
{
  std::wcout << termcolor::reset;
  std::wcout << "NowPlay v1.2 (build " << BUILD_NUMBER << ", " << __DATE__ << " " << __TIME__ << ")" << std::endl;
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

      if(filesystem::is_regular_file(name) && (name.extension() == ".mp3" || name.extension() == ".mp4"))
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
void playFiles(std::vector<FileInformation> &files)
{
  std::sort(files.begin(), files.end(), [](const FileInformation &lhs, const FileInformation &rhs) { return lhs.first < rhs.first; });

  auto playFile = [](const FileInformation &f)
  {
    std::wcout << termcolor::grey << termcolor::on_white << f.first.filename().wstring() << termcolor::reset;

    const bool isVideoFile = f.first.extension() == ".mp4";

    const std::string subtitleParams = isVideoFile ? " --subtitle-scale 1.3 ":"";

    const auto command = std::string("echo off & castnow \"") + f.first.string() + "\"" +  subtitleParams + " --quiet";
    std::system(command.c_str());

    std::wcout << "\r" << f.first.filename().wstring() << std::endl;
  };

  std::for_each(files.cbegin(), files.cend(), playFile);
}

//-----------------------------------------------------------------------------
std::vector<FileInformation> getCopyDirectories(std::vector<FileInformation> &dirs, const unsigned long long size)
{
  std::vector<FileInformation> selectedDirs;

  std::wcout << "Selecting from base for " << size << " bytes..." << std::endl;

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

  unsigned long long accumulator = 0L;
  auto printInfo = [&accumulator](const FileInformation &f)
  {
    std::wcout << "Selected: " << f.first.filename().wstring() << " (" << f.second << ")" << std::endl;
    accumulator += f.second;
  };
  std::for_each(selectedDirs.cbegin(), selectedDirs.cend(), printInfo);
  std::wcout << "Total bytes " << accumulator << " in " << selectedDirs.size() << " directories, remaining to limit " << remaining << " bytes." << std::endl;

  return selectedDirs;
}

//-----------------------------------------------------------------------------
void copyDirectories(const std::vector<FileInformation> &dirs, std::string &to)
{
  if(dirs.empty())
  {
    std::wcout << "ERROR: No directories to copy." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if(to.empty() || !filesystem::exists(to) || !filesystem::is_directory(to))
  {
    std::wcout << "ERROR: No destination directory to copy to." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  filesystem::directory_entry toEntry(to);

  std::wcout << "Copying files... ";

  system::error_code error;

  for(auto dir: dirs)
  {
    const auto newFolder = toEntry.path().wstring() + numeric_cast<wchar_t>('/') + dir.first.filename().wstring();
    filesystem::create_directory(newFolder);

    const auto files = getPlayableFiles(dir.first.string());

    for(auto file: files)
    {
      auto fullPath = newFolder + numeric_cast<wchar_t>('/') + file.first.filename().wstring();
      filesystem::copy_file(file.first, fullPath, error);

      if(error.value() != 0)
      {
        std::wcout << "ERROR: Cannot continue file copy. " << error.message().c_str() << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }
  }

  std::wcout << "done." << std::endl;
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  /** Puts the code page in UTF-8 but crashes in some cases. Better to output wrong characters and don't crash. */
  // SetConsoleOutputCP(CP_UTF8);

  banner();

  unsigned long long size = 0L;
  std::string destination;

  program_options::options_description desc("Options");
  desc.add_options()
        ("help,h", "Print application help message.")
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

    program_options::notify(vm);
  }
  catch (program_options::error &e)
  {
    std::cout << desc << std::endl;
    std::cout << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  auto isCopyMode = (size != 0L) || !destination.empty();

  auto directory = filesystem::current_path();

  std::wcout << "Base directory: " << directory.wstring();

  auto validPaths = getSubdirectories(directory.string(), isCopyMode);

  // Copy mode
  if(isCopyMode)
  {
    std::wcout << std::endl;

    if(size == 0L)
    {
      std::wcout << "ERROR: Invalid size option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    if(destination.empty() || !filesystem::exists(destination))
    {
      std::wcout << "ERROR: Invalid destination option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    if(validPaths.empty())
    {
      std::wcout << "ERROR: No subdirectories to select from." << std::endl;
      return EXIT_FAILURE;
    }

    auto selectedDirs = getCopyDirectories(validPaths, size*MEGABYTE);

    if(!selectedDirs.empty())
    {
      copyDirectories(selectedDirs, destination);
    }
    else
    {
      std::wcout << "ERROR: Unable to select directories for the given size: " << size << " Mb." << std::endl;
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  // Cast mode.
  if(!validPaths.empty())
  {
    std::wcout << " (" << validPaths.size() << " directories)" << std::endl;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);

    std::uniform_int_distribution<int> distribution(1, validPaths.size());
    int roll = distribution(generator);

    directory = validPaths.at(roll-1).first;

    std::wcout << "Selected: " << termcolor::grey << termcolor::on_white << directory.filename().wstring() << termcolor::reset << std::endl;
  }
  else
  {
    std::wcout << "\r" << "Base directory: " << termcolor::grey << termcolor::on_white << directory.wstring() << termcolor::reset << std::endl;
  }

  auto files = getPlayableFiles(directory.string());

  if(!files.empty())
  {
    playFiles(files);
  }
  else
  {
    std::wcout << "No music files found in directory: " << directory.wstring() << std::endl;
  }

  return EXIT_SUCCESS;
}
