/* Copyright 2021 Marzocchi Alessandro

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once
#include <Qt>
#include <QIODevice>
#include <QSharedPointer>

struct sqlite3;

namespace HFSQtLi
{
  /// \cond INTERNAL
  namespace Helper
  {
    class BlobData;
  }
  /// \endcond INTERNAL

  class Query;
  /**
   * @brief Class that gives access to a SQLite connection (struct sqlite3).
   *
   * No public constructor is available, instance of the class must be created via Db::open.
   */
  class Db
  {
    friend class Query;
    friend class Helper::BlobData;
  public:
    /**
     * @brief open Opens a database
     * @param filename Path to filename to open
     * @param An or between flags QIODevice::ReadWrite for a read/write database and QIODevice::Append to open only an existing database
     * @param errorMsg Pointer to a string that will be filled with error message in case of error
     * @param zVfs Virtual file system to open (See SQLite documentation)
     * @return A pointer to the opened database in case of success
     */
    static Db *open(const QString &filename,
                    QIODevice::OpenMode flags=QIODevice::ReadWrite, QString *errorMsg=nullptr,
                    const char *zVfs=nullptr);
    virtual ~Db();
    /**
     * @brief isOk Check the status of the last operation on database
     * @return True if last error is SQLITE_OK
     */
    bool isOk() const;
    /**
     * @brief error Check the error code from last operation on database
     * @return SQLITE error code
     */
    int error() const;
    QString errorMsg() const;
    /**
     * @brief query Creates a new query
     * @param queryStr Query string
     * @param persistent Optimize persistent queries
     * @param keepErrorMessage True if error message returned should be stored
     * @return The prepared query on success
     */
    Query *query(const char *queryStr, bool persistent=true, bool keepErrorMessage=true);
    /**
     * @copydoc query
     * @brief queryRef Same as \ref Db::query but returns a shared pointer
     */
    inline QSharedPointer<Query> queryRef(const char *queryStr, bool persistent=true, bool keepErrorMessage=true) { return QSharedPointer<Query>(query(queryStr, persistent, keepErrorMessage)); }

    /// @name Commands query execution
    /// The following function allows easy operation on any query that returns no row (e.g. CREATE TABLE or DELETE).
    /// See \ref Query::executeCommand.
    /// @{

    /// @brief Executes a query returning no rows. See \ref Query::executeCommand.
    /// @returns 0 on failure, 1+number of bound parameters on success
    template <typename... Args> inline int execute(const QString &query, Args &&... args) { return execute(nullptr, query, std::forward<Args>(args)...); }
    /// @brief Executes a query returning no rows, optionally returning the error message generated. See \ref Query::executeCommand.
    /// @returns 0 on failure, 1+number of bound parameters on success
    template <typename... Args> inline int execute(QString *error, const QString &query, Args &&... args);
    /// @}
    ///


    /// @name Single row query execution
    /// The following function allows easy operation on any query that returns one and only one row.
    /// They perform in one operation binding of parameters, a step on first row, fetching the data of the row and a second step to check the query effetively returned only one row.
    ///
    /// \note All the function in this group will return an error if the query returns no row or more than one row.
    /// @{

    /// @copydoc Query::executeSingleAllGeneric
    /// @param query A string containing the query to execute
    template <typename... Args> inline int executeSingleAll(const QString &query, Args &&... args) { return executeSingleAll<0>(nullptr, query, std::forward<Args>(args)...); }
    /// @copydoc Query::executeSingleAllGeneric
    /// @param query A string containing the query to execute
    template <int I, typename... Args> int executeSingleAll(const QString &query, Args &&... args) { return executeSingleAll<I>(nullptr, query, std::forward<Args>(args)...); }
    /// @copydoc Query::executeSingleAllGeneric
    /// @param error An optional pointer to a string to fill with the error message.
    /// @param query A string containing the query to execute
    template <int I, typename... Args> int executeSingleAll(QString *error, const QString &query, Args &&... args);
    /// @}

    /// \cond INTERNAL
    constexpr sqlite3 *internalDb() { return m_db; }
    class Lock
    {
    public:
      Lock(Db *db, bool lock);
      Lock(Lock &lock)=delete;
      Lock(Lock &&lock)=delete;
      ~Lock();
      void release();
      // Note: msg will be written only when it's not a success message (e.g. SQLITE_DONE or SQLITE_OK, SQLITE_ROW)
      void release(QString &msg);
      inline void release(QString *msg){ if(msg)release(*msg); else release(); }

      // Note: msg will be written only when the return code is not a success message (e.g. SQLITE_DONE or SQLITE_OK, SQLITE_ROW)
      void release(int code, QString &msg);
      // Note: msg will be written only when the return code is not a success message (e.g. SQLITE_DONE or SQLITE_OK, SQLITE_ROW)
      int releaseInternal(int code, QString &msg);
      // Note: msg will be written only when the return code is not a success message (e.g. SQLITE_DONE or SQLITE_OK, SQLITE_ROW)
      int releaseInternal(int code, QString &msg, const char *explicitMessage);
      // Same as release but writes to message in any case
      void releaseError(QString &msg);
      inline bool isHeld() { return m_db!=nullptr; }
    protected:
      sqlite3 *m_db;
    };
    /// \endcond INTERNAL
  protected:
    Db(const QString &filename, QIODevice::OpenMode flags=QIODevice::ReadWrite, const char *zVfs=NULL);
    sqlite3 *m_db;
    std::atomic_int m_queryCount;
    // Note: these variables are used only when open fails (m_db is null)
    int m_openError;
    QString m_openErrorMsg;
  };

}

#include "database_template.h"
