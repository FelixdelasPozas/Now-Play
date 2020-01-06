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

const std::string SEPARATOR = "/";

//-----------------------------------------------------------------------------
void banner()
{
  std::cout << termcolor::reset;
  std::cout << "NowPlay v1.1 (build " << BUILD_NUMBER << ", " << __DATE__ << " " << __TIME__ << ")" << std::endl;
}

//-----------------------------------------------------------------------------
std::vector<std::string> getSubdirectories(DIR *dir)
{
  std::vector<std::string> directories;

  if(dir != nullptr)
  {
    rewinddir(dir);

    struct dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr)
    {
      const std::string entry(ent->d_name);
      if(entry.compare(".") == 0 || entry.compare("..") == 0) continue;

      const auto fileat = GetFileAttributesA(ent->d_name);

      if(fileat == INVALID_FILE_ATTRIBUTES) continue;

      if(fileat & FILE_ATTRIBUTE_DIRECTORY)
      {
        directories.push_back(std::move(entry));
      }
    }
  }

  return directories;
}

//-----------------------------------------------------------------------------
std::vector<std::string> getPlayableFiles(DIR *dir)
{
  std::vector<std::string> files;

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
      const auto fileat = GetFileAttributesA(entryName.c_str());

      if(fileat == INVALID_FILE_ATTRIBUTES)
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
        std::cout << "Failed to get file attributes with error " << dw << ". Message: " <<  (LPCTSTR)lpMsgBuf << std::endl;
        std::cout << "Filename: " << entryName << std::endl;

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
        std::exit(1);
      }

      const bool isMusicFile = entry.substr(entry.length() -3).compare("mp3") == 0;
      const bool isVideoFile = entry.substr(entry.length() -3).compare("mp4") == 0;

      if(!(fileat & FILE_ATTRIBUTE_DIRECTORY) && (isMusicFile || isVideoFile))
      {
        files.push_back(std::move(entryName));
      }
    }
  }

  return files;
}

//-----------------------------------------------------------------------------
void playFiles(std::vector<std::string> &files)
{
  std::sort(files.begin(), files.end());

  auto playFile = [](const std::string &f)
  {
    const auto pos = f.rfind(SEPARATOR);
    std::cout << termcolor::grey << termcolor::on_white << f.substr(pos+1) << termcolor::reset;

    const bool isVideoFile = f.substr(f.length() -3).compare("mp4") == 0;

    const std::string subtitleParams = isVideoFile ? " --subtitle-scale 1.3 ":"";

    const auto command = std::string("echo off & castnow \"") + f + "\"" +  subtitleParams + " --quiet";
    system(command.c_str());

    std::cout << "\r" << f.substr(pos+1) << std::endl;
  };

  std::for_each(files.cbegin(), files.cend(), playFile);
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  banner();

  DIR *dir = nullptr;
  if ((dir = opendir(".")) != nullptr)
  {
    auto directory = std::string(dir->dd_name);
    directory = directory.substr(0, directory.length()-2);

    std::cout << "Base directory: " << directory;

    auto validPaths = getSubdirectories(dir);

    if(!validPaths.empty())
    {
      closedir(dir);

      std::cout << " (" << validPaths.size() << " directories)" << std::endl;

      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::default_random_engine generator (seed);

      std::uniform_int_distribution<int> distribution(1, validPaths.size());
      int roll = distribution(generator);

      const auto selectedPath = validPaths.at(roll-1);

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
      std::cout << std::endl;
      rewinddir(dir);
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
