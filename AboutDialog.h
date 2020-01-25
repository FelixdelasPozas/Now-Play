/*
 File: AboutDialog.h
 Created on: 22/01/2020
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

#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

// Qt
#include <QDialog>
#include "ui_AboutDialog.h"

class QKeyEvent;

/** \class AboutDialog
 * \brief Implements the ego-centric dialog.
 *
 */
class AboutDialog
: public QDialog
, private Ui::AboutDialog
{
  public:
    /** \brief AboutDialog class constructor.
     * \param[in] parent Raw pointer of the QWidget parent of this one.
     * \param[in] flags Window flags.
     *
     */
    explicit AboutDialog(QWidget *parent);

    /** \brief AboutDialog class virtual destructor.
     *
     */
    virtual ~AboutDialog()
    {}
};

#endif // ABOUTDIALOG_H_
