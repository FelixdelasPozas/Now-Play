/*
 File: CopyThread.h
 Created on: 11/02/2020
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

#ifndef COPYTHREAD_H_
#define COPYTHREAD_H_

// Project
#include <Utils.h>

// Qt
#include <QThread>

class CopyThread
: public QThread
{
    Q_OBJECT
  public:
    /** \brief CopyThread class constructor.
     * \param[in] selectedDirs List of selected directories to copy.
     * \param[in] destination Destination directory path.
     * \param[in] parent Raw pointer of the object parent of this one.
     *
     */
    explicit CopyThread(std::vector<Utils::FileInformation> selectedDirs, std::wstring destination, QObject *parent = nullptr);

    /** \brief CopyThread class virtual destructor.
     *
     */
    virtual ~CopyThread()
    {};

    /** \brief Stops the process.
     *
     */
    void stop()
    { m_abort = true; }

    /** \brief Returns true if the thread was aborted and false otherwise.
     *
     */
    bool isAborted() const
    { return m_abort; }

    /** \brief Returns the error message.
     *
     */
    QString errorMessage() const
    { return m_error; }

  signals:
    void log(const QString &message);
    void progress(int);

  protected:
    virtual void run();

  private:
    bool                                      m_abort;        /** true if aborted, false otherwise.  */
    QString                                   m_error;        /** error message or empty if success. */
    const std::vector<Utils::FileInformation> m_selectedDirs; /** list of directories to copy.       */
    const std::wstring                        m_destination;  /** destination directory.             */
};

#endif // COPYTHREAD_H_
