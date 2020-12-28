/*
 File: CopyThread.cpp
 Created on: 11/02/2020
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
#include <CopyThread.h>

// Qt
#include <QDir>

// C++
#include <filesystem>

//-----------------------------------------------------------------------------
CopyThread::CopyThread(std::vector<Utils::FileInformation> selectedDirs, std::wstring destination, QObject *parent)
: QThread(parent)
, m_abort(false)
, m_selectedDirs(selectedDirs)
, m_destination(destination)
{
}

//-----------------------------------------------------------------------------
void CopyThread::run()
{
  unsigned long long accumulator = 0L;
  auto printInfo = [&accumulator, this](const Utils::FileInformation &f)
  {
    QString message = tr("Selected: ") + QString::fromStdWString(f.first.filename().wstring()) + " (" + QString::number(f.second) + ")";
    log(message);

    accumulator += f.second;
  };
  std::for_each(m_selectedDirs.cbegin(), m_selectedDirs.cend(), printInfo);

  auto message = tr("Total bytes ") + QString::number(accumulator) + " in " + QString::number(m_selectedDirs.size()) + " directories.";
  emit log(message);

  emit log(tr("Copying directories..."));

  int i = 0;
  emit progress(0);

  for(auto dir: m_selectedDirs)
  {
    if(m_abort) return;

    emit progress((100*i)/m_selectedDirs.size());

    emit log(tr("Copying: %1").arg(QDir::toNativeSeparators(QString::fromStdWString(dir.first.wstring()))));

    if(!Utils::copyDirectory(dir.first.string(), m_destination))
    {
      m_error = QString("Error while copying files of directory: ") + QString::fromStdWString(dir.first.wstring());
      return;
    }

    ++i;
  }

  emit log(tr("Copy finished!"));

  emit progress(100);
}
