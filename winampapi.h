/*
		File: winampapi.h
    Created on: 20/01/2020
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


#ifndef WINAMPAPI_H_
#define WINAMPAPI_H_

// C++
#include <unistd.h>
#include <winbase.h>
#include <windef.h>
#include <winuser.h>
#include <cstring>
#include <iostream>
#include <string>

namespace WinAmp
{
  // NOTE: fixed location to avoid using an ini file or entering the path in the console.
  const std::string WINAMP_LOCATION = "D:\\Program Files (x86)\\Winamp\\winamp.exe";

  // Winamp WM_COMMAND & structs
  const int IPC_GETVERSION = 0;
  const int IPC_PLAYFILE   = 100;
  const int IPC_DELETE     = 101;
  const int IPC_STARTPLAY  = 102;

  /** \brief Returns the WinAmp handler.
   *
   */
  HWND getWinAmpHandle()
  {
    auto handler = FindWindow("Winamp v1.x",nullptr);

    if(!handler)
    {
      STARTUPINFOA info = { sizeof(STARTUPINFOA) };
      PROCESS_INFORMATION processInfo;
      if (CreateProcessA(0, const_cast<char *>(WINAMP_LOCATION.c_str()), 0, 0, 0, DETACHED_PROCESS|CREATE_NEW_PROCESS_GROUP, 0, 0, &info, &processInfo))
      {
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        int i = 0;
        while(!handler && i < 10)
        {
          sleep(1);
          handler = FindWindow("Winamp v1.x",nullptr);
          ++i;
        }
      }
      else
      {
        auto error = GetLastError();

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

        std::string message(messageBuffer, size);

        std::cout << "error code: " << error << " message: " << message << std::endl;

        LocalFree(messageBuffer);
      }
    }

    if(handler)
    {
      auto version = SendMessage(handler, WM_USER, 0, WinAmp::IPC_GETVERSION);

      std::cout << "Detected Winamp " << std::hex << ((version & 0x0000FF00) >> 12) << "." << (version & 0x000000FF) << std::endl;
    }

    return handler;
  }

  /** \brief Deletes the current WinAmp playlist.
   * \param[in] handler WinAmp handler.
   *
   */
  void deletePlaylist(HWND handler)
  {
    if(handler) SendMessage(handler, WM_USER, 0, IPC_DELETE);
  }

  /** \brief Adds a file to WinAmp playlist. File can be an audio file or a playlist.
   * \param[in] handler WinAmp handler.
   * \param[in] file File or playlist file absolute file path.
   *
   */
  void addFile(HWND handler, const std::string &file)
  {
    if(handler && !file.empty())
    {
      COPYDATASTRUCT pl = {0};
      pl.dwData = IPC_PLAYFILE;
      pl.lpData = (void*)file.c_str();
      pl.cbData = lstrlen((char*)pl.lpData)+1;

      SendMessage(handler ,WM_COPYDATA,0,(LPARAM)&pl);
    }
  };

  /** \brief Starts WinAmp play.
   * \param[in] handler WinAmp handler.
   *
   */
  void startPlay(HWND handler)
  {
    if(handler) SendMessage(handler, WM_USER, 0, IPC_STARTPLAY);
  }
}

#endif // WINAMPAPI_H_
