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
#include "templatehelper.h"

struct sqlite3_value;
/// @brief Global namespace for library
namespace HFSQtLi
{
  class Db;
  class Blob;
  class Query;

  /**
   * @brief Native types handled by SQLite
   */
  enum class Type: int
  {
    Invalid,
    Null,
    Integer,
    Float,
    Text,
    Blob
  };
  /**
   * @brief Accessor passed to custom data type bind
   * An instance of this class will be passed as first parameter to customBind and customBindConst functions (See \ref bindcustomtypes).
   *
   * The function will be able to use it to bind the columns with value from the object given as second parameter.
   *
   * Example usage:
   * \code
   * struct MyType
   * {
   *   int a;
   *   QString b;
   * };
   *
   * void customBindConst(CustomBind &bind, const MyType &val)
   * {
   *   bind.bind(val.a, val.b);
   * }
   *
   * ...
   *   int x;
   *   QString y;
   *   db->executeSingleAll<1>("SELECT $1, $2", MyType{1, "Hello, world"}, x, y);
   *   qDebug()<<x<<y; // Prints 1 "Hello, world"
   * ...
   * \endcode
   *
   */
  class CustomBind
  {
    friend class Query;
  public:
    /**
     * @brief Binds some values
     * This function is roughly equivalent to calling Query::bind. It will bind one or more values.
     * Note that the function will start binding from the column following the last bound column.
     * E.g. in:
     * \code
     * ... qry.bind(4, 3, MyCustomType);
     *
     *  void customBindConst(CustomBind &bind, const MyCustomType &val)
     * {
     *   bind.bind(12); // This will bind 3rd column
     *   bind.bind("Hello"); // This will bind 4th column
     * }
     *
     * \endcode
     * @param value Is a list of objects to bind
     * @return 0 on error, 1+number of bounded columns on failure
     */
    template <class ...T> inline int bind(T &&...value) { return bindIndex(1+numBound(), std::forward<T>(value)...); }
    /**
     * @brief Binds some values
     * This function is roughly equivalent to calling Query::bind. It will bind one or more values.
     * Note that the function will start binding from the column following the last column bind by the query.
     * E.g. in:
     * \code
     * ... qry.bind(4, 3, MyCustomType);
     *
     *  void customBindConst(CustomBind &bind, const MyCustomType &val)
     * {
     *   bind.bindIndex(2. "Hello"); // This will bind 4th column
     *   bind.bindIndex(1, 12);      // This will bind 3rd column
     * }
     *
     * \endcode
     * @param i 1-based index of first column to bind. Note that the 0 column is the one currently binding
     * @param value Is a list of objects to bind
     * @return 0 on error, 1+number of bounded columns on failure
     */
    template <class ...T> inline int bindIndex(int i, T &&...value);
    /**
     * @brief Gets the number of bound columns so far by this custom accessor.
     * Number of bound column is defined as the maximum column index bound minus the first column index associated with the accessor plus one. If no column has yet be bound returns 0.
     * @return The number of columns bound so far, or -1 if any error occoured in one of the binds
     */
    constexpr int numBound() { return m_bound; }
  protected:
    constexpr CustomBind(Query *query, int base): m_query(query), m_base(base), m_bound(0) { }
    Query *m_query;
    int m_base;
    int m_bound;
  };

  /**
   * @brief Accessor passed to custom data type fetch
   * An instance of this class will be passed as first parameter to customFetch functions (See \ref fetchcustomtypes).
   *
   * The function will be able to use it to bind the columns with value from the object given as second parameter.
   *
   * Example usage:
   * \code
   * struct MyType
   * {
   *   int a;
   *   QString b;
   * };
   *
   * void customFetch(CustomFetch &fetch, MyType &val)
   * {
   *   fetch.fetch(val.a, val.b);
   * }
   *
   * ...
   *   MyType x;
   *   db->executeSingleAll<2>("SELECT $1, $2", 3, "Hello, world!", x);
   *   qDebug()<<x.a<<x.b; // Prints 1 "Hello, world"
   * ...
   * \endcode
   *
   */  class CustomFetch
  {
    friend class Query;
  public:
    /**
     * @brief Fetches some values
     * This function is roughly equivalent to calling Query::column. It will fetchone or more values.
     * Note that the function will start fetch from the column following the last fetched column.
     * E.g. in:
     * \code
     * ... qry.fetch(a, b, MyCustomType);
     *
     *  void customBindConst(CustomBind &bind, const MyCustomType &val)
     * {
     *   int x,y;
     *   fetch.fetch(x); // This will fetch 3rd column
     *   fetch.fetch(y); // This will fetch 4th column
     * }
     *
     * \endcode
     * @param value Is a list of references to fetch
     * @return 0 on error, 1+number of bounded columns on failure
     */
    template <class ...T> inline int fetch(T &&...value) { return fetchIndex(m_strict, 0, std::forward<T>(value)...); }
    /**
     * @brief Fetches some values
     * This function is roughly equivalent to calling Query::column. It will fetch one or more values.
     * Note that the function will start fetchin from the column following the last column fetched by the query.
     * E.g. in:
     * \code
     * ... qry.bind(4, 3, MyCustomType);
     *
     *  void customBindConst(CustomBind &bind, const MyCustomType &val)
     * {
     *   int x, y;
     *   fetch.fetchIndex(1. y); // This will fetch 4th column
     *   fetch.fetchIndex(0, x); // This will fetch 3rd column
     * }
     *
     * \endcode
     * @param i 1-based index of first column to bind. Note that the 0 column is the one currently binding
     * @param value Is a list of objects to bind
     * @return 0 on error, 1+number of bounded columns on failure
     */
    template <class ...T> inline int fetchIndex(bool strict, int i, T &&...value);
    /**
     * @brief Gets the number of columns fetched so far by this custom accessor.
     * Number of fetched column is defined as the maximum column index fetched minus the first column index associated with the accessor plus one. If no column has yet be fetched returns 0.
     *
     * @return The number of columns fetched so far, or -1 if any error occoured in a fetch
     */
    constexpr int numFetched() { return m_fetched; }
  protected:
    constexpr CustomFetch(Query *query, bool strict, int base): m_query(query), m_strict(strict), m_base(base), m_fetched(0) { }
    Query *m_query;
    bool m_strict;
    int m_base;
    int m_fetched;
  };

  /// \cond INTERNAL
  namespace Custom
  {
    template <typename T> void customBindConst(CustomBind &bind, const T&value) = delete;
    template <typename T> void customBind(CustomBind &bind, T&&value) { return customBindConst(bind, const_cast<const T&>(value)); };
    template <typename T> void customFetch(CustomFetch &bind, T&&value) = delete;
  }


  namespace SQLiteCode
  {
    /// @brief Returns an error message even when code is success
    QString errorStringFull(int code);
    /// @brief Returns an error message in any case
    QString errorString(int code);
    // Returns true for codes considered a success (currently SQLITE_OK, SQLITE_DONE, SQLITE_ROW)
    /**
     * @brief Checks if an integer error is a successfull code
     *
     * Code that are considered ok are SQLITE_OK, SQLITE_DONE, SQLITE_ROW
     * @param code
     * @return True if code is a success one
     */
    bool isSuccess(int code);
    /// @brief Integer value corresonding to SQLITE_OK
    extern const int OK;
    /// @brief Integer value corresonding to SQLITE_DONE
    extern const int DONE;
    /// @brief Integer value corresonding to SQLITE_MISUSE
    extern const int MISUSE;
    /// @brief Integer value corresonding to SQLITE_CONSTRAINT
    extern const int CONSTRAINT;
    /**
     * @brief Converts an SQLite type to a \ref Type
     * @param type Type to convert
     * @return Converted type or Type::Invalid on error
     */
    Type typeFromSqlite(int type);
  }
  /// \endcond INTERNAL


  /**
   * @brief Class to skip fetching a column.
   * Example usage:
   * \code
   * int x;
   * qry.column(Unused(), x);
   * \endcode
   */
  typedef Helper::UnusedColumns<1> Unused;
  /** @brief Class to skip fetching a certain number of columns.
   *
   * Example usage:
   * \code
   * int x;
   * qry.column(UnusedN<2>(), x);
   * \endcode
   */
  template <unsigned U=1> using UnusedN=Helper::UnusedColumns<U>;

  // Column: fetches N columns and call a given functor with the read values.
  // Note: The functor must return true on success and false on error
  // Note: if any Unused is given it will not be passed forward as argument e.g. Call<Unused, int, Unused> will read 3 columns but make only the call f(int);
  /**
   * @brief Template struct used to pass a callback that will be called upon fetching values
   *
   * This struct will be instantiated with a list of types.
   * Passing it to a function fetching data it will read that set of data and upon succesfully fetching it call the callback given to the constructor.
   * Callback should have a signature:
   * *  bool function (T0 arg1, T1 arg2, ...)
   * Where T0, T1, ..., Tn are instantiaton types with Unused and UnusedN stripped
   *
   * The callback will have to return true on success or false on error
   *
   * Example usage:
   *  \code
   *  db->executeSingleAll(
   *    "SELECT 1,2,3,4",
   *    Call<int, UnusedN<2>, int> ([](int a, const int &b){ qDebug()<<a<<b; return true; }
   *  )); // Will print 1, 4
   * \endcode
   */
  template <typename ...T> struct Call
  {
    Call(const Helper::TypeUnusedRefBoolFunction<T...> &&f): m_function(std::move(f)) { }
    constexpr auto const &function() const { return m_function; }
  protected:
    Helper::TypeUnusedRefBoolFunction<T...> m_function;
  };

  /**
   * @brief Class to bind a NULL pointer (See \ref bindtypes).
   *
   * Example usage:
   * \code
   * qry.bind(4, Null());
   * \endcode
   */
  class Null
  {
  public:
    /// @brief Constructs a NULL value
    constexpr Null() { }
  };

  /**
   * @brief Maps a sqlite3_value type
   */
  class Value
  {
  public:
    inline constexpr Value(sqlite3_value *value=nullptr) : m_value(value) { }
    inline constexpr Value(Value &&other) : m_value(other.m_value) { other.m_value=nullptr; }
    Value &operator =(Value && other) { m_value=other.m_value; other.m_value=nullptr; return *this; }
    Value(const Value &other);
    inline ~Value() { if(m_value)clear(); }

    void clear();
    void dupFrom(const Value &other);

    double toDouble();
    qint64 toInt64();
    QString toString();

    bool isNull();
    bool isInt();
    bool isFloat();
    bool isText();
    bool isBlob();
    Type type();
    inline constexpr bool isValid() { return m_value!=nullptr; }
  protected:
    sqlite3_value *m_value;
  };
}

#include "util_template.h"
