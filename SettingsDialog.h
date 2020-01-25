/*
 File: SettingsDialog.h
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

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

// Project
#include "ui_SettingsDialog.h"

// Qt
#include <QDialog>

/** \class SettingsDialog
 * \brief Implements the application settins dialog.
 *
 */
class SettingsDialog
: public QDialog
, private Ui::SettingsDialog
{
    Q_OBJECT
  public:
    /** \brief SettingsDialog class constructor.
     * \param[in] winampPath Absolute path of WinAmp executable location.
     * \param[in] smplayerPath Absolute path of SMPlayer executable location.
     * \param[in] castnowPath Absolute path of Castnow script location.
     * \param[in] parent Raw pointer of the QWidget parent of this one.
     *
     */
    explicit SettingsDialog(const QString &winampPath, const QString &smplayerPath, const QString &castnowPath, QWidget *parent = nullptr);

    /** \brief SettingsDialog class virtual destructor.
     *
     */
    virtual ~SettingsDialog()
    {}

    /** \brief Returns the location of the Castnow script.
     *
     */
    const QString getCastnowLocation() const
    { return m_castnowPath->text(); }

    /** \brief Returns the location of the SMPlayer executable.
     *
     */
    const QString getSmplayerLocation() const
    { return m_smplayerPath->text(); }

    /** \brief Returns the location of the WinAmp executable.
     *
     */
    const QString getWinampLocation() const
    { return m_winampPath->text(); }

  private slots:
    /** \brief Browses for the given executable/script depending on the signal sender.
     *
     */
    void onBrowseButtonClicked();
};

#endif // SETTINGSDIALOG_H_
