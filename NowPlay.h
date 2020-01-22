/*
		File: NowPlay.h
    Created on: 21/01/2020
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

#ifndef NOWPLAY_H_
#define NOWPLAY_H_

// Project
#include <ui_NowPlayDialog.h>
#include <Utils.h>
#include <ProcessThread.h>

// Qt
#include <QDialog>

// Boost
#include <boost/filesystem.hpp>

// C++
#include <memory>

/** \class NowPlay
 * \brief Main application.
 *
 */
class NowPlay
: public QDialog
, private Ui::NowPlayDialog
{
    Q_OBJECT
  public:
    /** \brief NowPlay class constructor.
     *
     */
    explicit NowPlay();

    /** \brief NowPlay class virtual destructor.
     *
     */
    virtual ~NowPlay();

  private slots:
    /** \brief Changes the play button text to copy or viceversa depending on the current tab.
     * \param[in] index Current tab index.
     *
     */
    void onTabChanged(int index);

    /** \brief Plays, casts or copies depeding on the context.
     *
     */
    void onPlayButtonClicked();

    /** \brief Shows the about dialog.
     *
     */
    void onAboutButtonClicked();

    /** \brief Browse disk for a directory dialog.
     *
     */
    void browseDir();

    /** \brief Plays the next file on the list with Chromecast.
     *
     */
    void castFile();

  protected:
    /** \brief Sends the key presses to the casting process if running.
     * \param[in] e Key event.
     *
     */
    virtual void keyPressEvent(QKeyEvent *e) override;

  private:
    /** \brief Saves the application settings to the registry.
     *
     */
    void saveSettings();

    /** \brief Loads the application settings from the registry.
     *
     */
    void loadSettings();

    /** \brief Helper method to connect UI signals to slots.
     *
     */
    void connectSignals();

    /** \brief Shows an error message box on the screen.
     * \param[in] message Error message.
     * \param[in] title Message box title.
     * \param[in] details Error message details.
     *
     */
    void showErrorMessage(const QString &message, const QString &title = "Error", const QString &details = QString());

    /** \brief Plays the list of files with winamp.
     *
     */
    void callWinamp();

    /** \brief Writes the message to the log.
     * \param[in] message Text message.
     *
     */
    void log(const QString &message);

    /** \brief Returns true if the application can launch castnow application and false otherwise.
     *
     */
    bool hasCastnowInstalled();

    /** \brief Helper method that updates the GUI in constructor according to the application settings.
     *
     */
    void updateGUI();

    std::vector<Utils::FileInformation> m_files;  /** list of files being casted.                            */
    std::shared_ptr<ProcessThread>      m_thread; /** casting thread or nullptr if no file is being casted.  */
};


#endif // NOWPLAY_H_
