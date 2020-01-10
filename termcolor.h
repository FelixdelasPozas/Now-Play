/*
    File: termcolor.h
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

#ifndef TERMCOLOR_H_
#define TERMCOLOR_H_

// C++
#include <io.h>
#include <windows.h>
#include <iostream>
#include <cstdio>

namespace termcolor
{
  namespace _internal
  {
    inline bool is_valid_stream(const std::wostream &stream)
    {
      if(&stream == &std::wcout) return ::_isatty(_fileno(stdout));
      if(&stream == &std::wcerr) return ::_isatty(_fileno(stderr));

      return false;
    }

    inline bool is_valid_stream(const std::ostream &stream)
    {
      if(&stream == &std::cout) return ::_isatty(_fileno(stdout));
      if(&stream == &std::cerr) return ::_isatty(_fileno(stderr));

      return false;
    }

    void change_attributes(HANDLE terminal, int foreground, int background)
    {
      static WORD defaultAttributes = 0;

      // save default terminal attributes if it unsaved
      if (!defaultAttributes)
      {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(terminal, &info)) return;
        defaultAttributes = info.wAttributes;
      }

      // restore all default settings
      if (foreground == -1 && background == -1)
      {
        SetConsoleTextAttribute(terminal, defaultAttributes);
        return;
      }

      // get current settings
      CONSOLE_SCREEN_BUFFER_INFO info;
      if (!GetConsoleScreenBufferInfo(terminal, &info)) return;

      if (foreground != -1)
      {
        info.wAttributes &= ~(info.wAttributes & 0x0F);
        info.wAttributes |= static_cast<WORD>(foreground);
      }

      if (background != -1)
      {
        info.wAttributes &= ~(info.wAttributes & 0xF0);
        info.wAttributes |= static_cast<WORD>(background);
      }

      SetConsoleTextAttribute(terminal, info.wAttributes);
    }

    inline void change_console_attributes(std::wostream &stream, int foreground = -1, int background = -1)
    {
      if (!is_valid_stream(stream)) return;

      // get terminal handle
      HANDLE hTerminal = (&stream == &std::wcerr) ? GetStdHandle(STD_ERROR_HANDLE) : GetStdHandle(STD_OUTPUT_HANDLE);

      change_attributes(hTerminal, foreground, background);
    }

    inline void change_console_attributes(std::ostream &stream, int foreground = -1, int background = -1)
    {
      if (!is_valid_stream(stream)) return;

      // get terminal handle
      HANDLE hTerminal = (&stream == &std::cerr) ? GetStdHandle(STD_ERROR_HANDLE) : GetStdHandle(STD_OUTPUT_HANDLE);

      change_attributes(hTerminal, foreground, background);
    }

  } // namespace _internal

  inline std::ostream& reset(std::ostream &stream)
  {
    _internal::change_console_attributes(stream);
    return stream;
  }

  inline std::wostream& reset(std::wostream &stream)
  {
    _internal::change_console_attributes(stream);
    return stream;
  }

  inline std::ostream& grey(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, 0);
    return stream;
  }

  inline std::wostream& grey(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, 0);
    return stream;
  }

  inline std::ostream& red(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_RED);
    return stream;
  }

  inline std::wostream& red(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_RED);
    return stream;
  }

  inline std::ostream& green(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_GREEN);
    return stream;
  }

  inline std::wostream& green(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_GREEN);
    return stream;
  }

  inline std::ostream& yellow(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_GREEN | FOREGROUND_RED);
    return stream;
  }

  inline std::wostream& yellow(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_GREEN | FOREGROUND_RED);
    return stream;
  }

  inline std::ostream& blue(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE);
    return stream;
  }

  inline std::wostream& blue(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE);
    return stream;
  }

  inline std::ostream& magenta(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_RED);
    return stream;
  }

  inline std::wostream& magenta(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_RED);
    return stream;
  }

  inline std::ostream& cyan(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_GREEN);
    return stream;
  }

  inline std::wostream& cyan(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_GREEN);
    return stream;
  }

  inline std::ostream& white(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    return stream;
  }

  inline std::wostream& white(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    return stream;
  }

  inline std::ostream& on_grey(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, 0);
    return stream;
  }

  inline std::wostream& on_grey(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, 0);
    return stream;
  }

  inline std::ostream& on_red(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_RED);
    return stream;
  }

  inline std::wostream& on_red(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_RED);
    return stream;
  }

  inline std::ostream& on_green(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_GREEN);
    return stream;
  }

  inline std::wostream& on_green(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_GREEN);
    return stream;
  }

  inline std::ostream& on_yellow(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_GREEN | BACKGROUND_RED);
    return stream;
  }

  inline std::wostream& on_yellow(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_GREEN | BACKGROUND_RED);
    return stream;
  }

  inline std::ostream& on_blue(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE);
    return stream;
  }

  inline std::wostream& on_blue(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE);
    return stream;
  }

  inline std::ostream& on_magenta(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_RED);
    return stream;
  }

  inline std::wostream& on_magenta(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_RED);
    return stream;
  }

  inline std::ostream& on_cyan(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_GREEN);
    return stream;
  }

  inline std::wostream& on_cyan(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_GREEN);
    return stream;
  }

  inline std::ostream& on_white(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
    return stream;
  }

  inline std::wostream& on_white(std::wostream &stream)
  {
    _internal::change_console_attributes(stream, -1, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
    return stream;
  }

} // namespace termcolor

#endif // TERMCOLOR_H_
