/*
 File: Utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

// Boost
#include <boost/filesystem/path.hpp>

// Qt
#include <QString>

namespace Utils
{
  /** \brief Returns true if the given path is an audio file.
   * \param[in] path Absolute file path.
   *
   */
  bool isAudioFile(const boost::filesystem::path &path);

  /** \brief Returns true if the given path is a playlist file.
   * \param[in] path Absolute file path.
   *
   */
  bool isPlaylistFile(const boost::filesystem::path &path);

  /** \brief Returns true if the given path is a video file.
   * \param[in] path Absolute file path.
   *
   */
  bool isVideoFile(const boost::filesystem::path &path);

  using FileInformation = std::pair<boost::filesystem::path, uint64_t>;

  /** \brief Order operation for FileInformation items. Returns true if lhs is less than rhs and
   * false otherwise.
   * \param[in] lhs FileInformation object.
   * \param[in] rhs FileInformation object.
   *
   */
  bool lessThan(const FileInformation &lhs, const FileInformation &rhs);

  /** \brief Returns a list of playable files in the given directory.
   * \param[in] directory Absolute path of directory to search for playable files.
   *
   */
  std::vector<FileInformation> getPlayableFiles(const boost::filesystem::path &directory);

  /** \brief Returns a list of directories of the given base directory.
   * \param[in] directory Absolute path of directory to search for sub-directories.
   * \param[in] readSize True to read the sizes of the directories and false otherwise.
   *
   */
  std::vector<FileInformation> getSubdirectories(const boost::filesystem::path &directory, bool readSize = false);

  /** \brief Returns a random list of directories adjusted to the given size limit.
   * \param[in] dirs List of available directories.
   * \param[in] size Size limit in bytes.
   *
   */
  std::vector<FileInformation> getCopyDirectories(std::vector<FileInformation> &dirs, const unsigned long long size);

  /** \brief Copies the playable files of the given directory to the destination one.
   * \param[in] from Origin directory.
   * \param[in] to Destination directory.
   *
   */
  bool copyDirectory(const boost::filesystem::path &from, const boost::filesystem::path &to);

  /** \brief Helper method to check if WinAmp location is valid.
   * \param[in] location WinAmp location on disk.
   *
   */
  bool checkIfValidWinAmpLocation(const QString &location);

  /** \brief Helper method to check if video player location is valid.
   * \param[in] location Video player location on disk.
   *
   */
  bool checkIfValidVideoPlayerLocation(const QString &location);

  /** \brief Helper method to check if Castnow location is valid.
   * \param[in] location Castnow location on disk.
   *
   */
  bool checkIfValidCastnowLocation(const QString &location);
}

#endif // UTILS_H_
