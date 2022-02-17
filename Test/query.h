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
#include <QString>
#include "templatehelper.h"

struct sqlite3_stmt;
//#define SQLITE3_UNIVERSALREF(T, Type) class T, class=typename std::enable_if<std::is_same<typename std::decay<T>::type, Type>::value>::type

namespace HFSQtLi
{
  class Db;
  class Null;
  class ZeroBlob;
  class Blob;
  class Value;
  template <typename ...T> struct Call;

  enum class Type: int;
  /**
   * @brief Class that gives access to SQLite prepared statements (struct sqlite3_stmt)
   */
  class Query
  {
  public:
    friend class SqliteDb;
    friend class CustomBind;
    friend class CustomFetch;

   /// @name Constructors
   /// @{
   /**
   * @brief Construct a query prepared from a string
   * @copydetails Query(Db *, const QString &, bool, bool, QString *)
   */
    Query(Db *db, const char *query, bool persistent=false, bool storeErrorMsg=false, const char **tail=nullptr);
    /**
    * @brief Construct a query from a QString
    * @param db The database on which creating the query
    * @param query Query to perform
    * @param persistent See SQLite documentation
    * @param storeErrorMsg If true the string with the error message will be stored.
    * @param tail The part of the query that was not used
    */
    Query(Db *db, const QString &query, bool persistent=false, bool storeErrorMsg=false, QString *tail=nullptr);
    /**
    * @brief Construct a non prepared query
    * @param db The database on which creating the query
    * @param storeErrorMsg If true the string with the error message will be stored.
    */
    Query(Db *db=nullptr, bool storeErrorMsg=true);
    Query(const Query &) = delete;
    /// @}
    ~Query();

    /// @name General functions
    /// @{
    /**
     * @brief Prepares a query
     * @param query Query to be run
     * @param persistent See SQLite documentation
     * @param tail The part of the query that was not used
     * @return True on success
     */
    bool prepare(const QString &query, bool persistent=true, QString *tail=nullptr);
    /// @copydoc prepare(const QString &, bool, QString *)
    bool prepare(const char *query, bool persistent=true, const char **tail=nullptr);

    /**
     * @brief Move to next row in a query but do not fetch any data
     * @return True on success
     */
    bool stepNoFetch();
    /**
     * @brief Resets the query.
     * @return True on success
     */
    bool reset();
    /**
     * @brief Clear the bounded values of a query.
     * @return True on success
     */
    bool clearBindings();
    /**
     * @brief Finalizes a query. Query will have to be prepared again before being able to execute it again.
     * @return True on success.
     */
    bool finalize();

    /**
     * @brief Returns the SQLite error code of last function.
     * @param errorMsg If non-null the string will be set with error message from last function
     * @return The error code of last function
     */
    int error(QString *errorMsg=nullptr);
    /**
     * @brief Checks if last function call was sucessfull.
     * @return True if last function call was successfull.
     */
    bool isOk();
    /**
     * @brief Checks if last function call was the last step of a query.
     * @return True if last function call returned SQLITE3_DONE.
     */
    bool isDone();

    /**
     * @brief Gets the error message string.
     * @return The error message string. It may be empty if last execution was successfull (e.g. SQLITE3_DONE or SQLITE3_OK)
     */
    QString errorMsg() const;

    /**
     * @brief Check if the query contains a valid prepared statement
     * @return True if the query is a valid prepared statement
     */
    inline bool isPrepared() { return m_stmt; }
    /**
     * @brief Check if the query is associated with a valid and open database
     * @return True if the database associated to this query is valid and open
     */
    inline bool isDbValid();
    /**
     * @brief Changes the mode of retrival of error string.
     * If keep error message is true the exact error string returned by sqlite is fetches, otherwise the error message returned will be a generic one depending on the error code
     * @param newKeepErrorMsg Value to set
     */
    void setKeepErrorMsg(bool newKeepErrorMsg);
    /**
     * @brief Gets the keep error message status
     * @return Value of error message status
     */
    bool keepErrorMsg() const;
    /// @}

    /// @name Step-and-fetch functions
    /// All the function in this group advances the query to the next row and if this operation is successfull fetches some or all of its data.
    /// They are approximately equivalent to calling:
    /// \code
    ///   stepNoFetch() && columnXXX(args...);
    /// \endcode
    /// <br/>
    /// @{
    /**
     * @brief Performs a step on a query and fetches data after the step
     * @param index The index of the first column to retrive (0 is the first column)
     * @param args List of reference to variables that will be filled with columns starting at index
     * @return Zero on error or 1+number of fetched columns on success
     */
    template <typename... Args> int stepPartial(int index, Args &&... args) { int ret=stepGeneric(false, index, std::forward<Args>(args)...); return ret; }
    /**
     * @brief Performs a step on a query and fetches all the data
     * @param args List of reference to variables that will be filled with columns data
     * @return Zero on error or 1+number of fetched columns on success
     */
    template <typename... Args> int step(Args &&... args) { int ret=stepAllGeneric(false, std::forward<Args>(args)...); return ret; }

    /**
     * @brief Performs a step on a query and fetches data after the step with optional enabling of type checking (strict mode)
     * @param strict True if a strict fetch should be performed
     * @param index The index of the first column to retrive (0 is the first column)
     * @param args List of reference to variables that will be filled with columns starting at index
     * @return Zero on error or 1+number of fetched columns on success
     */
    template <typename... Args> int stepGeneric(bool strict, int index, Args &&... args)
    {
      int ret=stepNoFetch();
      if(ret)ret=columnHelper(strict, index, std::forward<Args>(args)...);
      return ret;
    }

    /**
     * @brief Performs a step on a query and fetches all the data after the step with optional enabling of type checking (strict mode)
     * @param strict True if a strict fetch should be performed
     * @param args List of reference to variables that will be filled with columns data
     * @return Zero on error or 1+number of fetched columns on success
     */
    template <typename... Args> int stepAllGeneric(bool strict, Args &&... args) {
      int ret=stepNoFetch();
      if(ret)ret=columnAllHelper(strict, std::forward<Args>(args)...);
      return ret;
    }
    /// @}

    /// @name Single row query execution
    /// The following function allows easy operation on any query that returns one and only one row.
    /// They perform in one operation binding of parameters, a step on first row, fetching the data of the row and a second step to check the query effetively returned only one row.
    /// For example:
    /// \code
    /// int x;
    /// qry.executeSingle<1>(4, x);
    /// \endcode
    /// Is more or less equivalent to:
    /// \code
    /// qry.bind(4) && qry.step() && qry.columnAll(x) && "qry.step() returns SQLITE3_DONE"
    /// \endcode
    /// Note All the function in this group will return an error if the query returns no row or more than one row
    /// @{

    /**
     * @copydoc executeSingleGeneric
     * @param index Index of data to fetch (0-based)
     */
    template <typename... Args> inline int executeSinglePartial(int index, Args &&... args) { return executeSingleGeneric<0>(false, 1, index, std::forward<Args>(args)...); }
    /// @copydoc executeSingleAllGeneric
    template <typename... Args> inline int executeSingle(Args &&... args) { return executeSingleAllGeneric<0>(false, std::forward<Args>(args)...); }
    /// @copydoc executeSingleGeneric
    template <int I, typename... Args> inline int executeSinglePartial(int indexBind, int indexFetch, Args &&... args) { return executeSingleGeneric<I>(false, indexBind, indexFetch, std::forward<Args>(args)...); }
    /// @copydoc executeSingleAllGeneric
    template <int I, typename... Args> inline int executeSingle(Args &&... args) { return executeSingleAllGeneric<I>(false, std::forward<decltype(args)>(args)...); }

    /// @brief Execute a query returning just one row and fetches some of the returned data checking data types.
    /// @param index Index of data to fetch (0-based)
    /// @copydetails executeSingleGeneric
    template <typename... Args> inline int executeSinglePartialStrict(int index, Args &&... args) { return executeSingleGeneric<0>(true, 1, index, std::forward<Args>(args)...); }
    /// @brief Execute a query returning just one row and fetches all of the returned data checking data types.
    /// @copydetails executeSingleAllGeneric
    template <typename... Args> inline int executeSingleStrict(Args &&... args) { return executeSingleAllGeneric<0>(true, std::forward<Args>(args)...); }
    /// @brief Execute a query returning just one row and fetches some of the returned data checking data types.
    /// @param index Index of data to fetch (0-based)
    /// @copydetails executeSingleGeneric
    template <int I, typename... Args> inline int executeSinglePartialStrict(int indexBind, int indexFetch, Args &&... args) { return executeSingleGeneric<I>(true, indexBind, indexFetch, std::forward<Args>(args)...); }
    /// @brief Execute a query returning just one row and fetches all of the returned data checking data types.
    /// @copydetails executeSingleAllGeneric
    template <int I, typename... Args> inline int executeSingleStrict(Args &&... args) { return executeSingleAllGeneric<I>(true, std::forward<Args>(args)...); }

    /**
     * @brief Execute a query returning just one row and fetches some of the returned data.
     * @param I Number of parameters to bind
     * @param strict True if the data of the types should be checked
     * @param indexBind Index of data to bind (1-based)
     * @param indexFetch Index of data to fetch (0-based)
     * @param args First I arguments are the values to bind (see \ref bindtypes), the ones after that are references to data to retrive (see \ref fetchtypes)
     * @return 0 on error, 1+number of fetched columns on success
     */
    template <int I, typename... Args> int executeSingleGeneric(bool strict, int indexBind, int indexFetch, Args &&... args);
    /**
     * @brief Execute a query returning just one row and fetches all of the returned data
     * @param I Number of parameters to bind
     * @param strict True if the data of the types should be checked
     * @param args First I arguments are the values to bind (see \ref bindtypes), the ones after that are references to data to retrive (see \ref fetchtypes)
     * @return 0 on error, 1+number of fetched columns on success
     * \warning Data is bound in temporary mode and resetBindings() is called before returning
     */
    template <int I, typename... Args> int executeSingleAllGeneric(bool strict, Args &&... args);

    /// @}


    /// @name Commands query execution
    /// The following function allows easy operation on any query that returns no row (e.g. CREATE TABLE or DELETE)
    /// They perform in one operation binding of parameters, and a step to run a query, checking that effectively the query returns no rows
    /// For example:
    /// \code
    /// qry.execute(4);
    /// \endcode
    /// Is more or less equivalent to:
    /// \code
    /// qry.bind(4) && "qry.step() returns SQLITE3_DONE"
    /// \endcode
    /// Note All the function in this group will return an error if the query returns any row.
    /// @{
    /**
     * @brief Executes a query returning no rows
     * @param args Values to bind (see \ref bindtypes)
     * @return 0 on error, 1+number of bound values on error
     * \warning Data is bound in temporary mode and resetBindings() is called before returning
     */
    template <typename... Args> inline int executeCommand(Args &&... args);
    /// @}



    /// @name Functions to bind data
    /// The functions here can be used to bind arguments while the query is in a reset state. For arguments see  \ref bindtypes .
    /// @{
    /**
     * @brief Binds some values in a query.
     * @param i First index to bind (1-based)
     * @param args Values to bind (see \ref bindtypes)
     * @return 0 on error, 1+number of bound columns on success
     */
    template <typename... Args> inline int bind(int i, Args &&...args) { return (bindSingleWithChecks(false, i, std::forward_as_tuple<decltype(args)...>(std::forward<Args>(args)...))); }
    /**
     * @brief Binds all values of a query.
     * @param args Values to bind (see \ref bindtypes)
     * @return 0 on error, 1+number of bound columns on success
     */
    template <typename... Args> inline int bindAll(Args &&...args) { int ret=bindSingleWithChecks(false, 1, std::tuple<decltype(args)...>(std::forward<Args>(args)...)); return (ret<=0)?0:(assertBindColumnCount(ret-1)+1); }
    /**
     * @brief Binds some values in a query. The existence of passed values must be guaranteed to persist until the binding is cleared (Query is destroyed or clearBindings() is called)
     * @copydetails bind
     */
    template <typename... Args> inline int bindTemporary(int i, Args &&...args) { return bindSingleWithChecks(true, i, std::forward_as_tuple<decltype(args)...>(std::forward<Args>(args)...)); }
    /**
     * @brief Binds all values. The existence of passed values must be guaranteed to persist until the binding is cleared (Query is destroyed or clearBindings() is called)
     * @copydetails bindAll
     */
    template <typename... Args> inline int bindTemporaryAll(Args &&...args) { int ret=bindSingleWithChecks(true, 1, std::forward_as_tuple<decltype(args)...>(std::forward<Args>(args)...)); return (ret<0)?0:(assertBindColumnCount(ret-1)+1); }
    /// @}

    /// @name Functions to fetch data
    /// The functions here can be used to retrived data while a query is currently pointing to a row. For a description of types accepted see \ref fetchtypes .
    /// @{
    /**
     * @brief Retrieves some columns from a row.
     * @param i Index of the first colum to fetch (0-based)
     * @param args Reference to data to retrive (see \ref fetchtypes)
     * @return 0 on error, 1+number of fetched columns on success
     */
    template <typename... Args> inline int column(int i, Args &&...args) { return columnHelper(false, i, std::forward<Args>(args)...); }
    /**
     * @brief Retrieves some columns from a row, returning error if types do not match.
     * @copydetails column
     */
    template <typename... Args> inline int columnStrict(int i, Args &&...args) { return columnHelper(true, i, std::forward<Args>(args)...); }
    /**
     * @brief Retrieves all columns from a row.
     * If the number of retrived columns does not match the columns returned by the query the function returns an error
     * @param args Reference to data to retrive (see \ref fetchtypes)
     * @return 0 on error, 1+number of fetched columns on success
     */
    template <typename... Args> inline int columnAll(Args &&...args) { return columnAllHelper(false, std::forward<Args>(args)...); }
    /**
     * @brief Retrieves all columns from a row, returning error if types do not match.
     * @copydetails columnAll
     */
    template <typename... Args> inline int columnStrictAll(Args &&...args) { return columnAllHelper(true, std::forward<Args>(args)...); }
    /// @}

    /// \cond INTERNAL
    constexpr sqlite3_stmt *pointerStatement() { return m_stmt; }
    constexpr Db *db() const { return m_db; }
    /// \endcond INTERNAL

  protected:
    // Note: function guarantees 0 will be returned on error
    template <typename... Args> inline int bindSingleWithChecks(bool temporary, int i, const std::tuple<Args...> &args);

    // Gets the error string from the database if m_error contains an error. Note: Mutex must be held when calling this function. Return true on success, false on failure
    inline bool fetchErrorString();
    void forceFetchErrorString();

    // Check if it is correct for i to be the last column binded and set error accordingly. Returns i on success, 0 on fail
    int assertBindColumnCount(int i);
    // Note: all next function expects that the db mutex is hold when calling them and that the query is valid. If an error is returned m_errorString should be set (if necessary)
    template <typename T> inline int bindSingle(bool, int i, T &&value );


    int bindSingle(bool, int i, std::nullptr_t);

    inline int bindSingle(bool temporary, int i, QByteArray &&value) { return bindSingle(temporary, i, const_cast<const QByteArray &>(value)); }
    inline int bindSingle(bool temporary, int i, QByteArray &value) { return bindSingle(temporary, i, const_cast<const QByteArray &>(value)); }
    int bindSingle(bool, int i, const QByteArray &value);

    // Should find a way to accomplish a single call:
    // template < SQLITE3_UNIVERSALREF(T, Null) > int bindSingle(bool temporary, int i, T &&) { return bindSingle(temporary, i, nullptr); }
    inline int bindSingle(bool temporary, int i, Null &&) { return bindSingle(temporary, i, nullptr); }
    inline int bindSingle(bool temporary, int i, const Null &) { return bindSingle(temporary, i, nullptr); }
    inline int bindSingle(bool temporary, int i, Null &) { return bindSingle(temporary, i, nullptr); }

    inline int bindSingle(bool temporary, int i, ZeroBlob &&value) { return bindSingle(temporary, i, const_cast<const ZeroBlob &>(value)); }
    inline int bindSingle(bool temporary, int i, ZeroBlob &value) { return bindSingle(temporary, i, const_cast<const ZeroBlob &>(value)); }
    int bindSingle(bool, int i, const ZeroBlob &zeroBlob);

    int bindSingle(bool, int i, qint64 value);
    int bindSingle(bool, int i, double value);

    int bindSingle(bool, int i, const char *value, int len=-1);
    inline int bindSingle(bool temporary, int i, int value) { return bindSingle(temporary, i, (qint64) value); }
    inline int bindSingle(bool temporary, int i, unsigned value) { return bindSingle(temporary, i, (qint64) value); }
    int bindSingle(bool temporary, int i, const QString &value);
    template <class ...T> int bindSingle(bool temporary, int i, const std::tuple<T...> &value) { return bindSingleHelper(temporary, i, value, Helper::make_int_sequence<sizeof...(T)>()); }
    template <class ...T> int bindSingle(bool temporary, int i, std::tuple<T...> &&value) { return bindSingle(temporary, i, static_cast<const std::tuple<T...> &>(value)); }
    template <class ...T, int ...I> int bindSingleHelper(bool temporary, int i, const std::tuple<T...> &value, Helper::int_sequence<I...>);

    Type columnType(int i);
    // Check if it is correct for i to be the last column fetched and set error accordingly. Returns i if the number of columns are correct, -1 otherwise
    int assertFetchColumnCount(int i);
    template <typename... Args> inline int columnHelper(bool strict, int i, Args &&...args);

    template <typename... Args> inline int columnAllHelper(bool strict, Args &&...args)
    {
      int ret=columnHelper(0, strict, std::forward<Args>(args)...);
      return (ret<=0?0:(assertFetchColumnCount(ret-1)+1));
    }
    // Sqlite-direct functions
    qint64 readColumnIntSQLite(int i, bool &ok);
    double readColumnDoubleSQLite(int i, bool &ok);
    // Helper function for reading an int
    template <class T> inline int readColumnInt(bool strict, int i, T &value);

  public:
    //    template <typename T> inline int readColumn(bool, int, T &&)=delete;
    // All readColumn returns the number of columns read, or -1 in case of error. These function are guaranteed to be called with a valid m_stmt
    template <typename T> inline int readColumn(bool strict, int i, T &&value);
    inline int readColumn(bool strict, int i, qint8 &value) { return readColumnInt<qint8>(strict, i, value); }
    inline int readColumn(bool strict, int i, quint8 &value) { return readColumnInt<quint8>(strict, i, value); }
    inline int readColumn(bool strict, int i, qint16 &value) { return readColumnInt<qint16>(strict, i, value); }
    inline int readColumn(bool strict, int i, quint16 &value) { return readColumnInt<quint16>(strict, i, value); }
    inline int readColumn(bool strict, int i, qint32 &value) { return readColumnInt<qint32>(strict, i, value); }
    inline int readColumn(bool strict, int i, quint32 &value) { return readColumnInt<quint32>(strict, i, value); }
    inline int readColumn(bool strict, int i, qint64 &value) { return readColumnInt<qint64>(strict, i, value); }
    inline int readColumn(bool strict, int i, quint64 &value) { return readColumnInt<quint64>(strict, i, value); }
    inline int readColumn(bool strict, int i, double &value);
    inline int readColumn(bool strict, int i, float &value);


    int readColumn(bool strict, int i, QString &value);
    int readColumn(bool strict,int i, Blob &value);
    int readColumn(bool strict, int i, QByteArray &value);
    int readColumn(bool, int i, Value &result);
    template <class T> inline int readColumn(bool strict, int i, std::optional<T> &result);

    template <class ...T> inline int readColumn(bool strict, int i, Call<T...> &call) { return readColumn(strict, i, static_cast<const Call<T...> &>(call)); }
    template <class ...T> inline int readColumn(bool strict, int i, Call<T...> &&call) { return readColumn(strict, i, static_cast<const Call<T...> &>(call)); }
    template <class ...T> inline int readColumn(bool strict, int i, const Call<T...> &call);

    template <unsigned I> constexpr int readColumn(bool, int, const Helper::UnusedColumns<I> &) { return I+1; }
    template <unsigned I> constexpr int readColumn(bool, int, Helper::UnusedColumns<I> &) { return I+1; }
    template <unsigned I> constexpr int readColumn(bool, int, Helper::UnusedColumns<I> &&) { return I+1; }

    template <class ...Args> int readColumn(bool strict, int i, const std::tuple<Args...> &values) { return readColumnHelper(strict, i, values, Helper::make_int_sequence<sizeof...(Args)>());}
    template <class ...Args> int readColumn(bool strict, int i, std::tuple<Args...> &values) { return readColumnHelper(strict, i, values, Helper::make_int_sequence<sizeof...(Args)>());}
    template <class ...Args> int readColumn(bool strict, int i, std::tuple<Args...> &&values) { return readColumnHelper(strict, i, std::forward<decltype(values)>(values), Helper::make_int_sequence<sizeof...(Args)>());}


    template <class ...Args, int ...Index> int readColumnHelper(bool strict, int i, std::tuple<Args...> &&values, Helper::int_sequence<Index...> seq) { return readColumnHelper(strict, i, const_cast<const std::tuple<Args...> &>(values), seq ); }
    template <class ...Args, int ...Index> int readColumnHelper(bool strict, int i, const std::tuple<Args...> &values, Helper::int_sequence<Index...>);
    template <class ...Args, int ...Index> int readColumnHelper(bool strict, int i, std::tuple<Args...> &values, Helper::int_sequence<Index...> seq);
    int readColumnInternal(int i, Blob &value, bool strict);

    // Clears the bindings without changing m_error. Used for resetting after temporary bindings (e.g. exec(...); )
    int clearBindingInternal();

    template <typename T> bool bindTemporary(int i, const T &v);
    template <typename T, typename... Args> bool bindTemporary(int i, const T &v, const Args &...args);

    void resetInternalError();
    void setInternalError(int code, const char *explicitMessag=nullptr);
    Db *m_db;
    sqlite3_stmt *m_stmt;
    int m_error;
    QString m_errorMsg;
    bool m_keepErrorMsg;
  };
}
#include "query_template.h"
