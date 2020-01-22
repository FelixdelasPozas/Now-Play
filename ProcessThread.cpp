/*
 File: ProcessThread.cpp
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
#include <ProcessThread.h>
#include <Utils.h>

//-----------------------------------------------------------------------------
ProcessThread::ProcessThread(const std::string &file, QObject *parent)
: QThread(parent)
, m_process(this)
, m_entity (file)
, m_abort  {false}
{
}

//-----------------------------------------------------------------------------
ProcessThread::~ProcessThread()
{
  if(m_process.state() == QProcess::ProcessState::Running)
  {
    m_process.close();
  }
}

//-----------------------------------------------------------------------------
void ProcessThread::sendKeyEvent(const QKeyEvent *event)
{
  if(event && (m_process.state() == QProcess::ProcessState::Running))
  {
    m_process.write(event->text().toStdString().c_str());
  }
}

//-----------------------------------------------------------------------------
void ProcessThread::run()
{
  const QString subtitleParams = Utils::isVideoFile(m_entity) ? " --subtitle-scale 1.3 ":"";
  const QString command = QString("castnow \"") + QString::fromStdString(m_entity) + "\"" +  subtitleParams + " --quiet";

  m_process.start("C:/windows/system32/cmd.exe",QStringList() << "/C" << command);
  m_process.waitForStarted();

  while(m_process.state() != QProcess::ProcessState::NotRunning && !m_abort)
  {
    QThread::sleep(1);
  }

  m_process.close();
}

//-----------------------------------------------------------------------------
void ProcessThread::stop()
{
  m_abort = true;
}
