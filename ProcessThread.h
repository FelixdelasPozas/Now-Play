/*
 File: ProcessThread.h
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

#ifndef PROCESSTHREAD_H_
#define PROCESSTHREAD_H_

// Qt
#include <QKeyEvent>
#include <QThread>
#include <QProcess>

/** \class ProcessThread
 * \brief Encapsulates a process executing castnow.
 *
 */
class ProcessThread
: public QThread
{
    Q_OBJECT
  public:
    /** \brief ProcessThread class constructor.
     * \param[in] file Absolute file path to media file to play.
     * \param[in] parent Raw pointer of the object parent of this one.
     *
     */
    explicit ProcessThread(const std::string &file, QObject *parent = nullptr);

    /** \brief ProcessThread class virtual destructor.
     *
     */
    virtual ~ProcessThread();

    /** \brief Send the key event to the process if executing.
     * \param[in] event Event to send.
     *
     */
    void sendKeyEvent(const QKeyEvent *event);

    /** \brief Stops the process and ends the thread.
     *
     */
    void stop();

  protected:
    virtual void run();

  private:
    QProcess          m_process; /** console process.                            */
    const std::string m_entity;  /** absolute path to media file.                */
    bool              m_abort;   /** true to abort process, and false otherwise. */
};

#endif // PROCESSTHREAD_H_
