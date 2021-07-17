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
#include <QFile>
#include <QTextStream>
#include <QStyle>

//-----------------------------------------------------------------------------
SettingsDialog::SettingsDialog(const PlayConfiguration &config, QWidget *parent)
: QDialog{parent}
{
  setWindowFlag(Qt::WindowContextHelpButtonHint,0);
  setupUi(this);

  m_winampPath->setText(QDir::toNativeSeparators(config.winampPath));
  m_castnowPath->setText(QDir::toNativeSeparators(config.castnowPath));
  m_videoPlayerPath->setText(QDir::toNativeSeparators(config.mplayerPath));
  m_continuousPlay->setChecked(config.continuous);

  connect(m_winampBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));
  connect(m_videoPlayerBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));
  connect(m_castnowBrowse, SIGNAL(pressed()), this, SLOT(onBrowseButtonClicked()));

  const auto theme = qApp->styleSheet();
  m_themeCombo->setCurrentIndex(theme.isEmpty() ? 0 : 1);

  connect(m_themeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onStyleComboChanged(int)));
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

//-----------------------------------------------------------------------------
void SettingsDialog::onStyleComboChanged(int idx)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QString sheet;

  if(idx != 0)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    sheet = ts.readAll();
  }

  qApp->setStyleSheet(sheet);

  QApplication::restoreOverrideCursor();
}
