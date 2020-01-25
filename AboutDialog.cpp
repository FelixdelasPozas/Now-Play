/*
 File: AboutDialog.cpp
 Created on: 11/08/2019
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
#include "AboutDialog.h"
#include "version.h"

// Qt
#include <QtGlobal>
#include <QKeyEvent>

// Boost
#include <boost/version.hpp>


//-----------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget* parent)
: QDialog(parent)
{
  setWindowFlag(Qt::WindowContextHelpButtonHint, 0);

  setupUi(this);

  m_qtVersion->setText(tr("version %1").arg(qVersion()));

  auto compilation_date = QString(__DATE__);
  auto compilation_time = QString(" (") + QString(__TIME__) + QString(")");

  const auto boostVersion = QString::number(BOOST_VERSION / 100000) + "." + QString::number(BOOST_VERSION / 100 % 1000) + "." + QString::number(BOOST_VERSION % 100);
  m_boostVersion->setText(tr("version %1").arg(boostVersion));

  m_compilationDate->setText(tr("Compiled on ") + compilation_date + compilation_time + " build " + QString::number(BUILD_NUMBER));
}

