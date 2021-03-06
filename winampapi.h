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

// Qt
#include <QString>
#include <QFileInfo>

namespace WinAmp
{
  // WinAmp WM_COMMAND & structs
  const int IPC_GETVERSION = 0;
  const int IPC_PLAYFILE   = 100;
  const int IPC_PLAYFILEW  = 1100;
  const int IPC_DELETE     = 101;
  const int IPC_STARTPLAY  = 102;

  /** \brief Returns the WinAmp handler.
   *
   */
  HWND getWinAmpHandle(const QString &winampPath)
  {
    auto handler = FindWindow("Winamp v1.x",nullptr);
    QFileInfo fi(winampPath);

    if(!handler)
    {
      if(fi.exists() && winampPath.endsWith("winamp.exe", Qt::CaseInsensitive))
      {
        STARTUPINFOA info = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION processInfo;
        if (CreateProcessA(0, const_cast<char *>(winampPath.toStdString().c_str()), 0, 0, 0, DETACHED_PROCESS|CREATE_NEW_PROCESS_GROUP, 0, 0, &info, &processInfo))
        {
          CloseHandle(processInfo.hProcess);
          CloseHandle(processInfo.hThread);

          std::cout << "Launched WinAmp. Waiting";

          int i = 0;
          while(!handler && i < 10)
          {
            sleep(1);
            std::cout << ".";
            handler = FindWindow("Winamp v1.x",nullptr);
            ++i;
          }
          std::cout << std::endl;
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
      else
      {
        std::cout << "Invalid Winamp command line path." << std::endl;
      }
    }

    if(handler)
    {
      auto version = SendMessage(handler, WM_USER, 0, WinAmp::IPC_GETVERSION);

      std::cout << "Detected Winamp " << std::hex << ((version & 0x0000FF00) >> 12) << "." << (version & 0x000000FF) << std::dec << std::endl;
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
  void addFile(HWND handler, const std::wstring &file)
  {
    if(handler && !file.empty())
    {
      COPYDATASTRUCT pl = {0};
      pl.dwData = IPC_PLAYFILEW;
      pl.lpData = (void*)file.data();
      pl.cbData = sizeof(wchar_t)*(wcslen(file.data())+1);

      SendMessage(handler, WM_COPYDATA,0,(LPARAM)&pl);
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
