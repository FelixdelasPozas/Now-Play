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

// Qt
#include <QDialog>

// Boost
#include <boost/filesystem.hpp>

class NowPlay
: public QDialog
, private Ui::NowPlayDialog
{
    Q_OBJECT
  public:
    explicit NowPlay();
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

  private:
    void loadSettings();
    void saveSettings();
    void connectSignals();

    void showErrorMessage(const QString &message, const QString &title = "Error", const QString &details = QString());

    const char SEPARATOR = '/';

    const unsigned long long MEGABYTE = 1024*1024;

    using FileInformation = std::pair<boost::filesystem::path, uint64_t>;

    static bool lessThan(const NowPlay::FileInformation &lhs, const NowPlay::FileInformation &rhs)
    {
      return lhs.first < rhs.first;
    }

    bool isAudioFile(const boost::filesystem::path &path);
    bool isPlaylistFile(const boost::filesystem::path &path);
    bool isVideoFile(const boost::filesystem::path &path);

    std::vector<FileInformation> getPlayableFiles(const std::string &directory);
    std::vector<FileInformation> getSubdirectories(const std::string &directory, bool readSize = false);
    void callWinamp(std::vector<FileInformation> &files);
    void castFiles(std::vector<FileInformation> &files);
    std::vector<FileInformation> getCopyDirectories(std::vector<FileInformation> &dirs, const unsigned long long size);
    void copyDirectories(const std::vector<FileInformation> &dirs, const std::string &to);

    void log(const QString &message);
};


#endif // NOWPLAY_H_
