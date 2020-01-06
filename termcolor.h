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
  enum class color: unsigned short
  {
    grey      = 0,
    red       = FOREGROUND_RED,
    green     = FOREGROUND_GREEN,
    yellow    = FOREGROUND_GREEN | FOREGROUND_RED,
    blue      = FOREGROUND_BLUE,
    magenta   = FOREGROUND_BLUE | FOREGROUND_RED,
    cyan      = FOREGROUND_BLUE | FOREGROUND_GREEN,
    white     = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
    unchanged = white + 1
  };

  //! Since C++ hasn't a way to hide something in the header from
  //! the outer access, I have to introduce this namespace which
  //! is used for internal purpose and should't be access from
  //! the user code.
  namespace _internal
  {
    inline bool is_valid_stream(const std::ostream &stream)
    {
      if(&stream == &std::cout) return ::_isatty(_fileno(stdout));
      if(&stream == &std::cerr) return ::_isatty(_fileno(stderr));

      return false;
    }

    inline void change_console_attributes(std::ostream &stream, color foreground, color background = color::unchanged)
    {
      static WORD defaultAttributes = 0;

      if (!is_valid_stream(stream)) return;

      // get terminal handle
      HANDLE hTerminal = (&stream == &std::cout) ? GetStdHandle(STD_OUTPUT_HANDLE) : GetStdHandle(STD_ERROR_HANDLE);

      // save default terminal attributes if it unsaved
      if (!defaultAttributes)
      {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(hTerminal, &info)) return;
        defaultAttributes = info.wAttributes;
      }

      // restore all default settings
      if (foreground == color::unchanged && background == color::unchanged)
      {
        SetConsoleTextAttribute(hTerminal, defaultAttributes);
        return;
      }

      // get current settings
      CONSOLE_SCREEN_BUFFER_INFO info;
      if (!GetConsoleScreenBufferInfo(hTerminal, &info)) return;

      if (foreground != color::unchanged)
      {
        info.wAttributes &= ~(info.wAttributes & 0x0F);
        info.wAttributes |= static_cast<WORD>(foreground);
      }

      if (background != color::unchanged)
      {
        info.wAttributes &= ~(info.wAttributes & 0xF0);
        info.wAttributes |= static_cast<WORD>(background);
      }

      SetConsoleTextAttribute(hTerminal, info.wAttributes);
    }

  } // namespace _internal

  inline std::ostream& reset(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::unchanged);
    return stream;
  }

  inline std::ostream& grey(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::grey);
    return stream;
  }

  inline std::ostream& red(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::red);
    return stream;
  }

  inline std::ostream& green(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::green);
    return stream;
  }

  inline std::ostream& yellow(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::yellow);
    return stream;
  }

  inline std::ostream& blue(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::blue);
    return stream;
  }

  inline std::ostream& magenta(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::magenta);
    return stream;
  }

  inline std::ostream& cyan(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::cyan);
    return stream;
  }

  inline std::ostream& white(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::white);
    return stream;
  }

  inline std::ostream& on_grey(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::grey);
    return stream;
  }

  inline std::ostream& on_red(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::red);
    return stream;
  }

  inline std::ostream& on_green(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::green);
    return stream;
  }

  inline std::ostream& on_yellow(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::yellow);
    return stream;
  }

  inline std::ostream& on_blue(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::blue);
    return stream;
  }

  inline std::ostream& on_magenta(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::magenta);
    return stream;
  }

  inline std::ostream& on_cyan(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::cyan);
    return stream;
  }

  inline std::ostream& on_white(std::ostream &stream)
  {
    _internal::change_console_attributes(stream, color::unchanged, color::white);
    return stream;
  }

} // namespace termcolor

#endif // TERMCOLOR_H_
