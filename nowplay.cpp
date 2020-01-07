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
#include <dirent.h>
#include <process.h>
#include <iostream>
#include <windows.h>
#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <ctime>
#include <chrono>

// Project
#include "termcolor.h"
#include "version.h"

// Boost
#include <boost/program_options.hpp>

const std::string SEPARATOR = "/";

const unsigned long long MEGABYTE = 1024*1024;

using FileInformation = std::pair<std::string, int64_t>;

using namespace boost;

//-----------------------------------------------------------------------------
void banner()
{
  std::cout << termcolor::reset;
  std::cout << "NowPlay v1.2 (build " << BUILD_NUMBER << ", " << __DATE__ << " " << __TIME__ << ")" << std::endl;
}

//-----------------------------------------------------------------------------
void reportError(const std::string &entryName)
{
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );

  // Display the error message and exit the process
  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
  std::cout << "ERROR: Failed to get file attributes of: " << entryName << ". Error " << dw << ". Message: " <<  (LPCTSTR)lpMsgBuf << std::endl;

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
}

//-----------------------------------------------------------------------------
std::vector<FileInformation> getPlayableFiles(DIR *dir)
{
  std::vector<FileInformation> files;

  if(dir != nullptr)
  {
    rewinddir(dir);

    auto directory = std::string(dir->dd_name);
    directory = directory.substr(0, directory.length()-2);

    struct dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr)
    {
      const std::string entry(ent->d_name, ent->d_namlen);
      if(entry.compare(".") == 0 || entry.compare("..") == 0) continue;

      const auto entryName = directory + SEPARATOR + entry;
      _WIN32_FILE_ATTRIBUTE_DATA fileInfo;
      if(!GetFileAttributesExA(entryName.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo))
      {
        std::cout << "ERROR: Unable to get information of: " << entryName << std::endl;
        reportError(entryName);
        continue;
      }

      if(fileInfo.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
      {
        std::cout << "ERROR: Invalid file attributes for: " << entryName << std::endl;
        reportError(entryName);
        continue;
      }

      const bool isMusicFile = entry.substr(entry.length() - 3).compare("mp3") == 0;
      const bool isVideoFile = entry.substr(entry.length() - 3).compare("mp4") == 0;

      if(!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (isMusicFile || isVideoFile))
      {
        int64_t size = fileInfo.nFileSizeHigh;
        size = (size << 32) | fileInfo.nFileSizeLow;

        files.emplace_back(entryName, size);
      }
    }
  }

  return files;
}

//-----------------------------------------------------------------------------
std::vector<FileInformation> getSubdirectories(DIR *dir, bool readSize = false)
{
  std::vector<FileInformation> directories;

  if(dir != nullptr)
  {
    rewinddir(dir);

    auto directory = std::string(dir->dd_name);
    directory = directory.substr(0, directory.length()-2);

    struct dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr)
    {
      const std::string entry(ent->d_name);
      if(entry.compare(".") == 0 || entry.compare("..") == 0) continue;

      const auto fileat = GetFileAttributesA(ent->d_name);

      if(fileat == INVALID_FILE_ATTRIBUTES) continue;

      if(fileat & FILE_ATTRIBUTE_DIRECTORY)
      {
        unsigned long long size = 0L;

        if(readSize)
        {
          const auto subDirName = directory + "/" + entry;
          DIR *subDir = opendir(subDirName.c_str());
          if (subDir == nullptr)
          {
            std::cout << "ERRROR: Unable to open subdirectory: " << subDirName << std::endl;
            std::exit(EXIT_FAILURE);
          }

          const auto files = getPlayableFiles(subDir);
          closedir(subDir);

          auto addOp = [](const unsigned long long &s, const FileInformation &f)
          {
            return s + f.second;
          };
          size = std::accumulate(files.cbegin(), files.cend(), 0L, addOp);
        }

        directories.emplace_back(entry, size);
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
    const auto &filename = f.first;
    const auto pos = filename.rfind(SEPARATOR);
    std::cout << termcolor::grey << termcolor::on_white << filename.substr(pos+1) << termcolor::reset;

    const bool isVideoFile = filename.substr(filename.length() -3).compare("mp4") == 0;

    const std::string subtitleParams = isVideoFile ? " --subtitle-scale 1.3 ":"";

    const auto command = std::string("echo off & castnow \"") + filename + "\"" +  subtitleParams + " --quiet";
    system(command.c_str());

    std::cout << "\r" << filename.substr(pos+1) << std::endl;
  };

  std::for_each(files.cbegin(), files.cend(), playFile);
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  banner();

  unsigned long long size = 0L;
  std::string destination;

  program_options::options_description desc("Options");
  desc.add_options()
        ("help", "Print application help message.")
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
  }
  catch (program_options::error &e)
  {
    std::cerr << desc << std::endl;
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if((size != 0L) || !destination.empty())
  {
    if(size == 0L)
    {
      std::cout << "ERROR: Invalid size option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    if(destination.empty())
    {
      std::cout << "ERROR: Invalid destination option value. Both a valid size and a destination are required." << std::endl;
      return EXIT_FAILURE;
    }

    size *= MEGABYTE;

    /* TODO: select random directories and adjust to the max to the given size and copy the files to the destination. */
  }

  DIR *dir = nullptr;
  if ((dir = opendir(".")) != nullptr)
  {
    auto directory = std::string(dir->dd_name);
    directory = directory.substr(0, directory.length()-2);

    std::cout << "Base directory: " << directory;

    auto validPaths = getSubdirectories(dir, false);

    if(!validPaths.empty())
    {
      closedir(dir);

      std::cout << " (" << validPaths.size() << " directories)" << std::endl;

      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::default_random_engine generator (seed);

      std::uniform_int_distribution<int> distribution(1, validPaths.size());
      int roll = distribution(generator);

      const auto selectedPath = validPaths.at(roll-1).first;

      std::cout << "Selected: " << termcolor::grey << termcolor::on_white << selectedPath << termcolor::reset << std::endl;

      directory += SEPARATOR + selectedPath;

      if ((dir = opendir(directory.c_str())) == nullptr)
      {
        std::cout << "Couldn't open the music directory: " << directory << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cout << "\r" << "Base directory: " << termcolor::grey << termcolor::on_white << directory << termcolor::reset << std::endl;
    }

    auto files = getPlayableFiles(dir);

    closedir(dir);
    dir = nullptr;

    if(!files.empty())
    {
      playFiles(files);
    }
    else
    {
      std::cout << "No music files found in directory: " << directory << std::endl;
    }
  }
  else
  {
    std::cout << "Couldn't open directory." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
