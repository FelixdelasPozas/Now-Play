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

// Qt
#include <QDialog>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QWinTaskbarButton>

// Boost
#include <boost/filesystem.hpp>

class QKeyEvent;
class QEvent;

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

  signals:
    void terminated();

  private slots:
    /** \brief Changes the play button text to copy or viceversa depending on the current tab.
     * \param[in] index Current tab index.
     *
     */
    void onTabChanged(int index);

    /** \brief Plays, casts or copies depending on the context.
     *
     */
    void onPlayButtonClicked();

    /** \brief Shows the about dialog.
     *
     */
    void onAboutButtonClicked();

    /** \brief Shows the configuration dialog.
     *
     */
    void onSettingsButtonClicked();

    /** \brief Browse disk for a directory dialog.
     *
     */
    void browseDir();

    /** \brief Plays the next file on the list with Chromecast.
     *
     */
    void castFile();

    /** \brief Monitors the cast output to detect when it has finished.
     *
     */
    void onOuttputAvailable();

    /** \brief Updates the size label when the subtitle value changes.
     * \param[in] value Size (subtitle value * 10).
     *
     */
    void onSubtitleSizeChanged(int value);

    /** \brief Plays the next file on the list, if any.
     *
     */
    void playNext();

    /** \brief Handles the icon tray activation.
     * \param[in] reason reason for activation.
     *
     */
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    /** \brief Restores the main dialog when the user double-clicks the tray icon.
     *
     */
    void onRestoreActionActivated();

  protected:
    virtual bool event(QEvent *e) override;

    virtual void keyPressEvent(QKeyEvent *e) override;

    virtual void changeEvent(QEvent *e) override;

    virtual void closeEvent(QCloseEvent *e) override;

    virtual void showEvent(QShowEvent *e) override;

    virtual void dropEvent(QDropEvent *e) override;

    virtual void dragEnterEvent(QDragEnterEvent *e) override;

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

    /** \brief Plays the list of files with SMPlayer.
     *
     */
    void playVideos();

    /** \brief Writes the message to the log.
     * \param[in] message Text message.
     *
     */
    void log(const QString &message);

    /** \brief Helper method that updates the GUI in constructor according to the application settings.
     *
     */
    void updateGUI();

    /** \brief Helper method to check if the application configurations are valid and modifies the GUI
     * depending on it.
     *
     */
    void checkApplications();

    /** \brief Helper method to setup the tray icon.
     *
     */
    void setupTrayIcon();

    /** \brief Sets the progress in the various widgets.
     * \param[in] value Progress value.
     *
     */
    void setProgress(int value);

    std::vector<Utils::FileInformation> m_files;         /** list of files being casted.      */
    QProcess                            m_process;       /** casting process.                 */
    QString                             m_winampPath;    /** WinAmp executable location.      */
    QString                             m_smplayerPath;  /** SMPlayer executable location.    */
    QString                             m_castnowPath;   /** Castnow script location.         */
    QSystemTrayIcon                    *m_icon;          /** application icon when minimized. */
    QWinTaskbarButton                  *m_taskBarButton; /** taskbar progress widget.         */
};


#endif // NOWPLAY_H_
