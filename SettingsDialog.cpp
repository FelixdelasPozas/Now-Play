/*
 File: SettingsDialog.cpp
 Created on: 25/01/2020
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
#include <SettingsDialog.h>

// Qt
#include <QDir>
#include <QFileDialog>

//-----------------------------------------------------------------------------
SettingsDialog::SettingsDialog(const QString &winampPath, const QString &smplayerPath, const QString &castnowPath, QWidget *parent)
: QDialog{parent}
{
  setWindowFlag(Qt::WindowContextHelpButtonHint,0);
  setupUi(this);

  m_winampPath->setText(QDir::toNativeSeparators(winampPath));
  m_castnowPath->setText(QDir::toNativeSeparators(castnowPath));
  m_videoPlayerPath->setText(QDir::toNativeSeparators(smplayerPath));

  connect(m_winampBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));
  connect(m_videoPlayerBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));
  connect(m_castnowBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));
}

//-----------------------------------------------------------------------------
void SettingsDialog::onBrowseButtonClicked()
{
  auto button = qobject_cast<QToolButton *>(sender());

  QString title;
  QString path;
  QString filter;

  if(button == m_winampBrowse)
  {
    title = tr("WinAmp Executable Location");
    path = m_winampPath->text();
    filter = tr("Executables (*.exe)");
  }
  else
  {
    if(button == m_videoPlayerBrowse)
    {
      title = tr("SMPlayer Executable Location");
      path = m_videoPlayerPath->text();
      filter = tr("Executables (*.exe)");
    }
    else
    {
      title = tr("Castnow Script Location");
      path = m_castnowPath->text();
      filter = tr("Script (*.cmd)");
    }
  }

#ifndef __WIN64__
  filter = QString();
#endif

  const auto file = QFileDialog::getOpenFileName(this, title, path, filter, &filter, QFileDialog::Option::ReadOnly);
  if(!file.isEmpty())
  {
    if(button == m_winampBrowse) m_winampPath->setText(QDir::toNativeSeparators(file));
    else if(button == m_videoPlayerBrowse) m_videoPlayerPath->setText(QDir::toNativeSeparators(file));
    else m_castnowPath->setText(QDir::toNativeSeparators(file));
  }
}
