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
#include <Utils.h>

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
    /** \struct PlayConfiguration
     * \brief Contains the playing settings.
     *
     */
    struct PlayConfiguration
    {
        QString musicPlayerPath; /** Music player path.                          */
        QString videoPlayerPath; /** Video player path.                          */
        QString castnowPath;     /** castnow executable path.                    */
        bool    continuous;      /** true if continuous play or false otherwise. */

        PlayConfiguration(): continuous{false} {};
    };

    /** \brief SettingsDialog class constructor.
     * \param[in] config PlayConfiguration struct.
     * \param[in] parent Raw pointer of the QWidget parent of this one.
     *
     */
    explicit SettingsDialog(const PlayConfiguration &config, QWidget *parent = nullptr);

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
    const QString getVideoPlayerLocation() const
    { return m_videoPlayerPath->text(); }

    /** \brief Returns the location of the WinAmp executable.
     *
     */
    const QString getMusicPlayerLocation() const
    { return m_musicPlayerPath->text(); }

    /** \brief Returns the value of the continuous play checkbox.
     *
     */
    const bool getContinuousPlay() const
    { return m_continuousPlay->isChecked(); }

  private slots:
    /** \brief Browses for the given executable/script depending on the signal sender.
     *
     */
    void onBrowseButtonClicked();

    /** \brief Changes the application stylesheet when the user changes the styles combo box.
     * \param[in] idx Combo current index value.
     *
     */
    void onStyleComboChanged(int idx);
};

#endif // SETTINGSDIALOG_H_
