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
#include <utility>
#include <functional>
#include <tuple>
#include <QString>
#include <QIODevice>
#include <QSharedPointer>
#include <QSharedData>



namespace HFSQtLi
{
  /// \cond INTERNAL
  namespace Helper
  {

    template <int ...Index> using int_sequence=std::integer_sequence<int, Index...> ;
    template<int OFFSET, int ...INDEX> constexpr auto make_int_sequenceHelper (int_sequence<INDEX...> ) { return int_sequence<(INDEX+OFFSET)...>(); }
    /// @brief constructs an integer sequence with indexes START, START+1, ..., START+SIZE-1
    template<int START, int SIZE> constexpr auto make_int_sequence () { static_assert(SIZE>=0, "Size of make_int sequence must be >0"); return make_int_sequenceHelper<START>(std::make_integer_sequence<int, SIZE>()); }
    /// @brief constructs an integer sequence with indexes 0, 1, ..., SIZE-1
    template<int SIZE> constexpr auto make_int_sequence () { static_assert(SIZE>=0, "Size of make_int sequence must be >0"); return std::make_integer_sequence<int, SIZE>(); }

    template <typename T> T & forwardSingleAsRef(T & value) { return value;};
    template <typename T> const T &forwardSingleAsRef(T && value) {return value;};
    template <typename T> const T & forwardSingleAsRef(const T & value) {return value;};
    template <typename T> const T & forwardSingleAsRef(const T && value) {return value;};
    ///@ Returned [i] will be of type T& if args[i] is a left reference, const T &otherwise
    template <typename ...T> constexpr auto forwardAsRef(T &&...arg)
    {
      return std::forward_as_tuple(forwardSingleAsRef(std::forward<T>(arg))...);
    }

//    template <typename ...T, int... Index> constexpr auto tupleToRef(const std::tuple<T...> &values, int_sequence<Index...>)
//    {
//      return std::forward_as_tuple(forwardSingleAsRef(std::forward<T>(std::get<Index>(values)))...);
//    }
//    template <typename ...T> constexpr auto tupleToRef(const std::tuple<T...> &values)
//    {
//      return tupleToRef(values, make_int_sequence<sizeof...(T)>());
//    }

    template <typename T> struct MyForwardTupleRefT { typedef T &type; };
    template <typename T> struct MyForwardTupleRefT<T&> { typedef T &type; };
    template <typename T> struct MyForwardTupleRefT<T&&> { typedef T &&type; };
    template <typename T> struct MyForwardTupleRefT<const T&&> { typedef const T &&type; };
    ///@ Returned type will be of type T& if args[i] is a value, the original type otherwise
    template <typename T> using MyForwardTupleRef = typename MyForwardTupleRefT<T>::type;


    template<typename ...T , int ...Index> auto tuple_forwardHelper(std::tuple<T...> &value, Helper::int_sequence<Index...>)
    {
      return std::tuple<MyForwardTupleRef<T>...>((MyForwardTupleRef<T>)std::get<Index>(value)...);
    }

    ///@ Returned tuple[i] will be a left reference to value[i] if value[i] is a value, the original reference otherwise
    template<typename ...T> auto tuple_forward(std::tuple<T...> &value)
    {
      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
    }

    template <typename T> struct MyForwardConstTupleRefT { typedef const T &type; };
    template <typename T> struct MyForwardConstTupleRefT<T&> { typedef T &type; };
    template <typename T> struct MyForwardConstTupleRefT<T&&> { typedef T &&type; };
    template <typename T> struct MyForwardConstTupleRefT<const T&&> { typedef const T &&type; };
    template <typename T> using MyForwardConstTupleRef = typename MyForwardConstTupleRefT<T>::type;
    template<typename ...T , int ...Index> auto tuple_forwardHelper(const std::tuple<T...> &value, Helper::int_sequence<Index...>)
    {
      return std::tuple<MyForwardConstTupleRef<T>...>(static_cast<MyForwardConstTupleRef<T>>(std::get<Index>(value))...);
    }
    template<typename ...T> auto tuple_forward(const std::tuple<T...> &value)
    {
      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
    }
//    template <typename T> struct MyForwardRRefTupleRefT { typedef T &&type; };
//    template <typename T> struct MyForwardRRefTupleRefT<T&> { typedef T &type; };
//    template <typename T> struct MyForwardRRefTupleRefT<T&&> { typedef T &&type; };
//    template <typename T> struct MyForwardRRefTupleRefT<const T&&> { typedef const T &&type; };
//    template <typename T> using MyForwardRRefTupleRef = typename MyForwardRRefTupleRefT<T>::type;

//    template<typename ...T , int ...Index> auto tuple_forwardHelper(const std::tuple<T...> &&value, Helper::int_sequence<Index...>)
//    {
//      return std::tuple<MyForwardRRefTupleRef<T>...>(static_cast<MyForwardRRefTupleRef<T>>(std::get<Index>(value))...);
//    }
//    template<typename ...T> auto tuple_forward(const std::tuple<T...> &&value)
//    {
//      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
//    }

    /// @brief Given a tuple returns a tuple created with the given subset of Indexes of the tuple
    template <typename ...T, int... Index> constexpr auto select(const std::tuple<T...> &tuple, Helper::int_sequence<Index...>)
    {
      using sourceTupleUnref=std::remove_const_t<std::remove_reference_t<decltype(tuple)>>;
      using type=std::tuple<std::tuple_element_t<Index, sourceTupleUnref>...>;
      return type(std::forward< std::tuple_element_t<Index, sourceTupleUnref>>(std::get<Index>(tuple))...);
    }

    template <unsigned I=1> struct UnusedColumns { };

    template <typename T, typename A, typename ...Ts> struct RemoveUnusedIndexes { };
    template <int ...Is, int ...As, typename ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<Is...>,Helper::int_sequence<As...>,Ts...>
    {
      static constexpr auto indexes() { return Helper::int_sequence<As...>(); }
    };
    template <int I, int ...Is, int ...As, class T, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, T, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As..., I>, Ts...>::indexes(); }
    };
    template <int I, int ...Is, int ...As, unsigned J, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, Helper::UnusedColumns<J>, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As...>, Ts...>::indexes(); }
    };
    template <int I, int ...Is, int ...As, unsigned J, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, Helper::UnusedColumns<J> &, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As...>, Ts...>::indexes(); }
    };
    template <typename T> struct StructRemoveUnusedItem
    {
      static T &&remove(T &&value) { return value; }
    };
    template <typename ...Ts> struct StructRemoveUnusedItem<std::tuple<Ts...> &>
    {
      template <int ...Is> static auto removeHelper(Helper::int_sequence<Is...>, std::tuple<Ts...> &data)
      {
        return std::tuple<decltype(StructRemoveUnusedItem<decltype(std::get<Is>(data))>::remove(std::get<Is>(data)))...>(StructRemoveUnusedItem<decltype(std::get<Is>(data))>::remove(std::get<Is>(data))...);
      }
      static auto remove(std::tuple<Ts...> &value)
      {
        auto i=RemoveUnusedIndexes<decltype(Helper::make_int_sequence<sizeof...(Ts)>()), Helper::int_sequence<>, Ts... >::indexes();
        return removeHelper(i, value);
      }
    };

    template <class ...Ts> auto removeUnused(std::tuple<Ts...> &data)
    {
      return StructRemoveUnusedItem<decltype(data) &&>::remove(data);
    }

    template <typename R, typename ...T> auto typeUnusedRefFunctionHelper2(const std::tuple<T...> &)
    {
      return std::function<R(T...)>();
    }

    template <class T> struct RemoveConstRefTupleT
    {
      typedef std::remove_const_t<std::remove_reference_t<T>> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< std::tuple<Ts...> >
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< const std::tuple<Ts...> &>
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< std::tuple<Ts...> &>
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };

    template <typename T> using RemoveConstRefTuple = typename RemoveConstRefTupleT<T>::type;

    template <typename R,typename ...T> auto typeUnusedRefFunctionHelper()
    {
      RemoveConstRefTuple<std::tuple<T...>> data;
      auto refPurgedData(Helper::removeUnused(data));
      return typeUnusedRefFunctionHelper2<R>(refPurgedData);
    }
    template <typename R, typename ...T> using TypeUnusedRefFunction=decltype(typeUnusedRefFunctionHelper<R, T...>());
    template <typename ...T> using TypeUnusedRefBoolFunction=decltype(typeUnusedRefFunctionHelper<bool, T...>());

  }
  /// \endcond INTERNAL
}

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


namespace HFSQtLi
{
  template <typename... Args> inline int Db::execute(QString *message, const QString &query, Args &&... args)
  {
    int ret=0;
    Query qry(this, message /*storeErrorMessage */);
    if( !qry.prepare(query, false) || !(ret=qry.executeCommand(std::forward<Args>(args)...)))
    {
      if(message)
        *message=qry.errorMsg();
    }
    return ret;

  }

  template <int I, typename... Args> int Db::executeSingleAll(QString *message, const QString &query, Args &&... args)
  {
    int ret=0;
    Query qry(this, message /*storeErrorMessage */);
    if( !qry.prepare(query, false) || !(ret=qry.executeSingle<I>(std::forward<Args>(args)...)))
    {
      if(message)
        *message=qry.errorMsg();
    }
    return ret;
  }
}
namespace HFSQtLi
{
  bool Query::isDbValid()
  {
    return m_db && m_db->m_db;
  }

  bool Query::fetchErrorString()
  {
    bool ret=SQLiteCode::isSuccess(m_error);
    if(m_keepErrorMsg && !ret) forceFetchErrorString();
    return ret;
  }

  template <typename T> int Query::bindSingle(bool, int i, T &&value )
  {
    CustomBind custom(this, i);
    using Custom::customBind;
    customBind(custom, std::forward<T>(value));
    return custom.numBound()+1;
  }

  template <typename T> int Query::readColumn(bool strict, int i, T &&value)
  {
    CustomFetch custom(this, strict, i);
    using Custom::customFetch;
    customFetch(custom, std::forward<T>(value));
    return custom.numFetched()+1;
  }

  template <class T> inline int Query::readColumn(bool strict, int i, std::optional<T> &result)
  {
    int ret=2;
    if(columnType(i)==Type::Null)
      result.reset();
    else
    {
      result.emplace();
      ret=readColumn(strict, i, *result);
    }
    return ret;
  }

  template <typename... Args> int Query::columnHelper(bool strict, int i, Args &&...args)
  {
    int ret=0;
    if(!m_stmt) setInternalError(SQLiteCode::MISUSE);
    else
    {
      ret=readColumn(strict, i, std::tuple<Args...>(std::forward<Args>(args)...));
    }
    return ret;
  }

  template <class T> inline int Query::readColumnInt(bool strict, int i, T &value)
  {
    bool ok=false;
    if(strict)
    {
      if(columnType(i)!=Type::Integer)
        setInternalError(SQLiteCode::CONSTRAINT, "Read column was not an integer");
      else
      {
        qint64 value64=readColumnIntSQLite(i, ok);
        if(ok)
        {
          if(value>std::numeric_limits<T>::max() || (value<0 && std::numeric_limits<T>::min()>=0) || value<std::numeric_limits<T>::min())
            setInternalError(SQLiteCode::CONSTRAINT, "Value outside variable range");
          else
            value=value64;
        }
      }
    }
    else
    {
      value=readColumnIntSQLite(i, ok);
    }
    return ok?2:0;
  }

  inline int Query::readColumn(bool strict, int i, double &value)
  {
    bool ok=false;
    if(strict)
    {
      if(columnType(i)!=Type::Float)
        setInternalError(SQLiteCode::CONSTRAINT, "Read column was not a floating point");
      else
        value=readColumnDoubleSQLite(i, ok);
    }
    else
    {
      value=readColumnDoubleSQLite(i, ok);
    }
    return ok?2:0;
  }

  inline int Query::readColumn(bool strict, int i, float &value)
  {
    bool ok=false;
    if(strict)
    {
      if(columnType(i)!=Type::Float)
        setInternalError(SQLiteCode::CONSTRAINT, "Read column was not a floating point");
      else
      {
        value=readColumnDoubleSQLite(i, ok);
        if(ok && !qIsNaN(value) && !qIsInf(value) && (value<-std::numeric_limits<float>::max() || value>std::numeric_limits<float>::max()))
        {
          setInternalError(SQLiteCode::CONSTRAINT, "Value outside float range");
          ok=false;
        }
      }
    }
    else
    {
      value=readColumnDoubleSQLite(i, ok);
    }
    return ok?2:0;
  }

  template <class ...T> inline int Query::readColumn(bool strict, int i, const Call<T...> &call)
  {
    int ret=0;
    Helper::RemoveConstRefTuple<std::tuple<T...>> data;
    ret=readColumn(strict, i, data);
    if(ret>0 && call.function())
    {
      auto x(Helper::removeUnused(data));
      resetInternalError();
      bool b=std::apply(call.function(),x);
      if(!b)
      {
        ret=0;
        if(m_error==SQLiteCode::OK)
          setInternalError(SQLiteCode::DONE);
      }
    }
    return ret;
  }

  template <typename... Args, int... Index> int Query::readColumnHelper(bool strict, int i, const std::tuple<Args...> &args, Helper::int_sequence<Index...>)
  {
    int curNumRead=0, numRead=0;
    (void)((curNumRead=readColumn(strict, i+numRead, std::get<Index>(args)), numRead+=curNumRead-1, curNumRead>0) &&...);
    return (curNumRead>0?numRead+1:0);
  }

  template <typename... Args, int... Index> int Query::readColumnHelper(bool strict, int i, std::tuple<Args...> &args, Helper::int_sequence<Index...>)
  {
    int curNumRead=0, numRead=0;
    (void)((curNumRead=readColumn(strict, i+numRead, static_cast<Helper::MyForwardTupleRef<Args>>(std::get<Index>(args))), numRead+=curNumRead-1, curNumRead>0) &&...);
    return (curNumRead>0?numRead+1:0);
  }

  template <class ...T, int ...I> int Query::bindSingleHelper(bool temporary, int i, const std::tuple<T...> &value, Helper::int_sequence<I...>)
  {
    Q_UNUSED(temporary);
    int curNumBound=1, numBound=0;
    (void)((curNumBound=bindSingle(temporary, i+numBound, std::forward<T>(std::get<I>(value))), numBound+=curNumBound-1, curNumBound>0) &&...);
    return (curNumBound>0?numBound+1:0);
  }


  template <typename... Args> inline int Query::bindSingleWithChecks(bool temporary, int i, const std::tuple<Args...> &args)
  {
    int ret=-1;
    m_errorMsg.clear();
    if(!isPrepared())
      setInternalError(SQLiteCode::MISUSE);
    else
    {
      Db::Lock lock(m_db, m_keepErrorMsg);
      ret=bindSingle(temporary, i, args);
    }
    return ret<=0?0:ret;
  }

  template <int I, typename... Args> int Query::executeSingleGeneric(bool strict, int indexBind, int indexFetch, Args &&... args)
  {
    int ret=0;
    static_assert (I<=sizeof...(Args), "stepSingle<I> number of bound parameters is greater than number of arguments");
    std::tuple<decltype(args)...> ref(std::forward<Args>(args)...);
    auto const toBind=Helper::select(ref, Helper::make_int_sequence<I>());
    auto const toFetch=Helper::select(ref, Helper::make_int_sequence<I, sizeof...(Args)-I>());
    if(reset() && bind(indexBind, toBind))
    {
      if((ret=stepGeneric(strict, indexFetch, toFetch)))
      {
        if(stepNoFetch()){ // A second step was successfull. Query does not return a single row
          setInternalError(SQLiteCode::CONSTRAINT, "Step single query returned more than one row");
          ret=0;
        }
        else if(!isDone()) // We had an error. If it is note SQLITE3_DONE returns an error
          ret=0;
      }
      else if(isDone())
        setInternalError(SQLiteCode::CONSTRAINT, "Step single query returned no rows");
    }
    return ret;
  }

  template <int I, typename... Args> int Query::executeSingleAllGeneric(bool strict, Args &&... args)
  {
    int ret=0;
    static_assert (I<=sizeof...(Args), "stepSingle<I> number of bound parameters is greater than number of arguments");
    std::tuple<decltype(args)...> ref(std::forward<Args>(args)...);
    auto const toBind=Helper::select(ref, Helper::make_int_sequence<I>());
    auto const toFetch=Helper::select(ref, Helper::make_int_sequence<I, sizeof...(Args)-I>());
    if(reset() && bindTemporaryAll(toBind))
    {
      if((ret=stepAllGeneric(strict, toFetch)))
      {
        if(stepNoFetch()){ // A second step was successfull. Query does not return a single row
          setInternalError(SQLiteCode::CONSTRAINT, "Step single query returned more than one row");
          ret=0;
        }
        else if(!isDone()) // We had an error. If it is note SQLITE3_DONE returns an error
          ret=0;
      }
      else if(isDone())
        setInternalError(SQLiteCode::CONSTRAINT, "Step single query returned no rows");
    }
    return ret;
  }

  template <typename... Args> inline int Query::executeCommand(Args &&... args)
  {
    int ret=0;
    if(reset() && (ret=bindTemporaryAll(std::forward<Args>(args)...)))
    {
      if(stepNoFetch())
      {
        setInternalError(SQLiteCode::CONSTRAINT, "Command query returned a row");
        ret=0;
      }
      else if(error()==SQLiteCode::DONE)
      {
        resetInternalError();
      }
      else
        ret=0;
    }
    clearBindingInternal();
    return ret;
  }

}

namespace HFSQtLi
{
  template <class ...T> int CustomBind::bindIndex(int i, T &&...value)
  {
    int ret=0;
    if(m_bound>=0)
    {
      if(i<=0)
      {
        m_query->setInternalError(SQLiteCode::MISUSE, "Custom bind with index <=0");
        m_bound=-1;
      }
      else
      {
        ret=0;
        ret=m_query->bindSingle(false, m_base+i-1, std::tuple<decltype(value)...>(std::forward<T>(value)...));
        if(ret)
          m_bound=qMax(m_bound, i+ret-2);
        else
          m_bound=-1;
      }
    }
    return ret;
  }

  template <class ...T> int CustomFetch::fetchIndex(bool strict, int i, T &&...value)
  {
    int ret=0;
    if(m_fetched>=0)
    {
      if(i<0)
      {
        m_query->setInternalError(SQLiteCode::MISUSE, "Custom fetch with index <0");
        m_fetched=-1;
      }
      else
      {
        ret=0;
        ret=m_query->readColumn(strict, m_base+i, std::tuple<decltype(value)...>(std::forward<T>(value)...));
        if(ret)
          m_fetched=qMax(m_fetched, i+ret-1);
        else
          m_fetched=-1;
      }
    }
    return ret;
  }
}
struct sqlite3_blob;

namespace HFSQtLi
{
  class Db;
  /**
   * @brief Placeholder for a zero intialized blob of a given size (See \ref bindtypes).
   *
   * Example usage:
   * \code
   * qry.bind(ZeroBlob(50));
   * \endcode
   */
  class ZeroBlob
  {
  public:
      /**
       * @brief Constructor
       * @param size Is the size of the blob to be created
       */
      constexpr ZeroBlob(qint64 size): m_size(size) { }
      constexpr qint64 size() const { return m_size; }
  protected:
    qint64 m_size;
  };
  /// \cond INTERNAL
  namespace Helper
  {
  /// @brief Implementation for Blob
  class BlobData: public QSharedData
  {
  public:
    ~BlobData();
    const char *database() const { return m_database; }
    const char *table() const { return m_table; }
    const char *column() const { return m_column; }
    qint64 rowId() const { return m_rowId; }
    inline bool readWrite() const { return m_readWrite; }
    inline bool autoOpen() const { return m_autoOpen; }
    bool read(void *data, qsizetype size, qsizetype offset);
    bool write(const void *data, qsizetype size, qsizetype offset);

    qint64 size();

    bool close(QString *errorMsg=nullptr);
    bool open(QString *errorMsg=nullptr);
    bool open(qint64 id, QString *errorMsg=nullptr);
    bool reopenFast(qint64 newId, QString *errorMsg=nullptr);
    inline bool isOpen() { return m_blob!=nullptr; }
    inline bool isValid() { return m_db && m_table && m_column && m_hasRowId; }
    BlobData(Db *connection, const char *database, const char *table, const char *column, bool readWrite, bool autoOpen);
    BlobData(const BlobData &)=delete;
    BlobData(const BlobData &&)=delete;

    bool setDbPointer(Db *db);
    bool setDatabase(const char *database);
    bool setTable(const char *table);
    bool setColumn(const char *column);
    bool setRowId(qint64 value);
    bool checkAutoOpen();

    inline bool setReadWrite(bool readWrite) { bool ret=false; if(!m_blob){ret=true; m_readWrite=readWrite; } return ret; }
    inline bool setAutoOpen(bool newAutoOpen) { m_autoOpen=newAutoOpen; return true; }

    //    BlobData(const char *database, const char *table, const char *column, bool readOnly, bool autoOpen);
    Db *m_db;
    sqlite3_blob *m_blob;
    char *m_database;
    char *m_table;
    char *m_column;
    qint64 m_rowId;
    bool m_hasRowId;
    bool m_readWrite;
    // Auto-open to true will mean that the blob will be opened when the blob is passed to readColumn
    bool m_autoOpen;
    qint64 m_sizeCache;
  };
  }
  /// \endcond INTERNAL

  /**
   * @brief Provides direct read/write access to a blob
   *
   * Blob has two options that influence its behaviour:
   *
   * * readWrite set if the blob should be open in read-only mode (false) or in read-write mode (true)
   * * autoOpen If set to true (default behaviour) after any call to function that sets parameters if all parameters needed for opening the blob (database pointer, table, column and row id) are set the function will try to open/reopen
   * the blob. Note that in this case the functions will return true only if the open was successfull.
   *
   * There are three ways of setting blob parameters:
   * * Directly using any function to set parameters calls
   * * Reading the Blob with a column containing a blob sets database pointer, database, table and column to the one of the blob
   * * Reading the Blob with a column containing an integer sets the rowid
   *
   * Typical usage case will be like setting everything by hand:
   * \code
   * Blob b; // Note that auto-open is enabled by default
   * b.set(m_db, nullptr, "TestTable", "blobColumn", 4); // "main" database selected by default
   * \endcode
   *
   * or set all the parameters except the rowid by hand and then using a "SELECT" to retive it
   * \code
   * Blob b; // Note that auto-open is enabled by default
   * b.set(m_db, nullptr, "TestTable", "blobColumn"); // "main" database selected by default
   * m_db->executeSingle("SELECT rowid FROM TestTable WHERE name="Foo", b)
   * \endcode
   *
   * or set all the parameters via a select:
   * \code
   * Blob b; // Note that auto-open is enabled by default
   * m_db->executeSingle("SELECT blobColumn, rowid FROM TestTable WHERE name="Foo", b, b);
   * \endcode
   */
  class Blob
  {
  public:
    /**
     * @brief Constructor.
     * @param readWrite Read/write mode. See \ref isReadWrite()
     * @param autoOpen. See \ref autoOpen()
     */
    Blob(bool readWrite=false, bool autoOpen=true): m_data(new Helper::BlobData(nullptr, nullptr, nullptr, nullptr, readWrite, autoOpen)) { }
    Blob(Blob &source): m_data(source.m_data) { }

    /**
     * @brief Closes the blob if open.
     * @param errorMsg Optional pointer to a string that will receive the error message
     * @return true on success or if the blob was already closed
     */
    inline bool close(QString *errorMsg=nullptr) { return m_data->close(errorMsg); }

    /**
     * @brief Opens the blob if closed.
     * @param errorMsg Optional pointer to a string that will receive the error message
     * @return true on success or if the blob was already opened
     */
    inline bool open(QString *errorMsg=nullptr) { return m_data->open(errorMsg); }

    /**
     * @brief Open the blob to a particular row id.
     *
     * If the blob is closed tries to open it with the given id, if it is already open tries to reopen it with the new id.
     * @param errorMsg Optional pointer to a string that will receive the error message
     * @return true on success, false on failure
     */
    inline bool open(qint64 rowid, QString *errorMsg=nullptr) { return m_data->open(rowid,errorMsg); }

    /**
     * @brief Reopens the blob with a new id
     *
     * This function must be called on an open blob or it will return an error
     * @param errorMsg Optional pointer to a string that will receive the error message
     * @return true on success
     */
    inline bool reopenFast(qint64 newId, QString *errorMsg=nullptr) { return m_data->reopenFast(newId, errorMsg); }

    /**
     * @brief Checks if the blob is currently open
     * @return True if the blob is open, false otherwise
     */
    inline bool isOpen() { return m_data->isOpen(); }

    /**
     * @brief Returns the read/write mode.
     *
     * If false the blob is in read-only mode and any write call will fail.
     * @return Read/write status
     */
    inline bool isReadWrite() { return m_data->readWrite(); }
    /**
     * @brief Returns the autoOpen mode
     *
     * If true the blob will try to automatically open the blob when all of db pointer, tableName, columnName and rowid are set.
     * @return autoOpen status
     */
    inline bool autoOpen() { return m_data->autoOpen(); }

    /**
     * @brief Get the size of the blob
     * @return Size of the blob on success, -1 on error or if the blob is closed
     */
    inline qint64 size() { return m_data->size(); }
    /**
     * @brief Reads data from the blob.
     * @param data Pointer to the buffer to read
     * @param size Size of the buffer to read
     * @param offset Offset in blob to read
     * @return True on success, false on error
     */
    inline bool read(void *data, qsizetype size, qsizetype offset=0) { return m_data->read(data, size, offset); }
    /**
     * @brief Read data from the blob an returns a QByteArray with read data.
     * @param size Size of the buffer to read
     * @param offset Offset in blob to read
     * @return QByteArray filled with read data, or an empty one on error
     */
    inline QByteArray read(qsizetype size, qsizetype offset=0) { QByteArray ret; ret.resize(size); if(!read(ret.data(), size, offset)) ret.clear(); return ret; }
    /**
     * @brief Reads all the data in the blob.
     *
     * This call is equivalent to read(size(), 0);
     * @return QByteArray fill with the data from blob or an empty one on error
     */
    inline QByteArray readAll() { return read(size()); }

    /**
     * @brief Writes data to the blob.
     * @param data Pointer to the buffer to write
     * @param size Size of the buffer to write
     * @param offset Offset in the blob to write
     * @return True on success, false on error
     */
    inline bool write(const void *data, qsizetype size, qsizetype offset) { return m_data->write(data, size, offset); }
    /**
     * @brief Writes data to the blob.
     * @param data Data to write
     * @param offset Offset in the blob to write
     * @return True on success, false on error
     */
    inline bool write(const QByteArray &data, qsizetype offset=0) { return write(data.data(), data.size(), offset); }

    /**
    *   @name Functions to set parameters
    *   The functions here are used to set all parameters needed for succesfully opening a blob.
    *   \warning Calling any function in this group with values differents from the ones already set closes the Blob if autoOpen is not true or reopens it if it is false
    * @{ */
    /**
     * @brief Sets several parameters at once
     * @param db Database pointer
     * @param database Name of the database
     * @param table Name of table
     * @param column Name of column
     * @param rowid Row id
     * @return True on success, false on error
     */
    bool set(Db *db, const char *database, const char *table, const char *column, qint64 rowid);
    /**
     * @brief Sets several parameters at once
     * @param db Database pointer
     * @param database Name of the database
     * @param table Name of table
     * @param column Name of column
     * @return True on success, false on error
     */
    bool set(Db *db, const char *database, const char *table, const char *column);
    /**
     * @brief Sets several parameters at once
     * @param db Database pointer
     * @param database Name of the database
     * @param table Name of table
     * @param rowid Row id
     * @return True on success, false on error
     */
    bool set(Db *db, const char *database, const char *table, qint64 rowid);

    /**
     * @brief Sets the database name. If not set it will "main"
     *
     * @param database String pointing to database name or NULL to unset.
     * @return True on success, false on error
     */
    inline bool setDatabase(const char *database) { return m_data->setDatabase(database); }
    /**
     * @brief Sets the table name.
     *
     * @param table String pointing to table name or NULL to unset.
     * @return True on success, false on error
     */
    inline bool setTable(const char *table) { return m_data->setTable(table); }
    /**
     * @brief Sets the column name.
     *
     * @param table String pointing to column name or NULL to unset.
     * @return True on success, false on error
     */
    inline bool setColumn(const char *column) { return m_data->setColumn(column); }
    /**
     * @brief Sets the row id.
     *
     * @param rowid Integer of the row id.
     * @return True on success, false on error
     */
    inline bool setRowId(qint64 rowId) { return m_data->setRowId(rowId); }
    /**
     * @brief Sets the read/write mode.
     *
     * @param readWrite New value.
     * @return True on success, false on error
     */
    inline bool setReadWrite(bool readWrite) { return m_data->setReadWrite(readWrite); }
    /**
     * @brief Sets the connection this blob refers to
     * @param db Pointer to an open connection
     * @return True on success
     */
    inline bool setDbPointer(Db *db) { return m_data->setDbPointer(db); }

    /**
     * @brief Sets the autoOpen mode
     *
     * Note that setting autoOpen to true will not try to open the blob until the next call to setXXX.
     * @param newAutoOpen New value.
     */
    inline void setAutoOpen(bool newAutoOpen) { m_data->setAutoOpen(newAutoOpen); }
    /// @}

    /**
     * @brief Returns current database name
     * @return String with database name
     */
    inline QString database() { return QString::fromUtf8(m_data->database()?m_data->database():"main"); }
    /**
     * @brief Returns current table name
     * @return String with table name or empty string if unset
     */
    inline QString table() { return QString::fromUtf8(m_data->table()); }
    /**
     * @brief Returns current column name
     * @return String with column name or empty string if unset
     */
    inline QString column() { return QString::fromUtf8(m_data->column()); }
    /**
     * @brief Returns current row id
     * @return Current row id. Result is undefined if no row-id was set.
     */
    inline qint64 rowId() { return m_data->rowId(); }
  protected:
    QExplicitlySharedDataPointer<Helper::BlobData> m_data;
  };
}

namespace HFSQtLi
{

/** @mainpage HFSQtLi
 *
 * @section intro What is HFSQtLi
 *
 * HFSQtLi is a Qt/C++ wrapper for SQLite3. It tries to use templating to make the use as intuitive as possible while at the same time mapping (almost) directly to just
 * a few C calls to the right functions of SQLite API.
 *
 * @section howtouse Examples of usage
 * @subsection helloworld Hello, world!
 * \code
 *   QString str;
 *   auto *db=Db::open(":memory:", QIODevice::ReadWrite);
 *   db->executeSingleAll <1> (
 *     "SELECT \"Hello, \" || $1 || \"!\"; ",
 *     "world",
 *     str
 *   );
 *   qDebug()<<str; // Prints "Hello, world!"
 * \endcode
 * @subsection fetchrows Fetching rows
 * \code
 *   Query qry(db);
 *   if(db.prepare("SELECT a FROM testTable"))
 *   {
 *     QString val;
 *     while(query.step(val))
 *       qDebug()<<val;
 *   }
 * \endcode
 * @subsection errormessage Retriving error messages
 * \code
 *   Query qry(db);
 *   if(qry.prepare("SLECT a FROM testTable")) // Note: invalid query string
 *   {
 *     ...
 *   }
 *   else
 *     qDebug()<<qry.errorMsg();
 * \endcode
 * @subsection customdatatypeexample Custom data types
 * See examples of \ref CustomBind and \ref CustomFetch
 */

/** @page fetchtypes Fetched data types
 *  This section describes the data types that are handled by the fetching functions (e.g. Query::step, Db::executeSingle, ...).
 *  Fetching a data type comes in two flavors: standard and strict. The strict mode checks for the type and range of retrived column to be compatible before fetching the data.
 *  If anything does not match an error will be returned by the corresponding call.
 *  The standard mode on the other hand just calls the API of the requested column type without performings any checks.
 *  @section fetchnativedatatypes Native data types
 *  Following types are directly mapped to sqlite_column_X calls
 *  \code
 *    int x;
 *    double y;
 *    db->executeSingleAll("SELECT 3, 4.5", x, y); // x is 3, y is 4.5
 *  \endcode
 *  @subsection fetchinttypes Integer types
 *  The following types are handled natively:
 *  - qint8
 *  - quint8
 *  - qint16
 *  - quint16
 *  - qint32
 *  - quint32
 *  - qint64
 *  - quint64
 *
 *  If retriving in strict mode the retrived column is checked to be of integer type and the range of the destination type is checked (e.g. fetching 129 in a qint8 will give an error).
 *
 *  Note: as SQLite stores only 64 bit signed integers the results of operations on 64 bit unsigned with the most signficant bit set is undefined
 *  @subsection fetchfloattypes Floating point
 *  The following types are handled natively:
 *  - float
 *  - double
 *
 *  In strict mode the column is checked to be a floating point. Also the float version checks that absolute value of retrived column is not between float_max and infinite (not inclusive)
 *  @subsection fetchstringbytearray Other native types
 *  The following types are handled natively:
 *  - QString
 *  - QByteArray
 *
 *  Text and blob data can be read respectively with QString and QByteArray
 *  @section fetchcpptypes C++ data types
 *  @subsection fetchoptional std::optional<T>
 *  If the fetched column is NULL the result is cleared, othewise the value will be read as if the type T was read diredtly.
 *  \code
 *    std::optional<int> x,y;
 *    db->executeSingleAll("SELECT NULL, 3", x, y); // x is empty, y is 3
 *  \endcode
 *  @subsection fetchtuple std::tuple<T...>
 *  If the fetched data is a tuple then the data is retrived as if each element of the tuple was retrived consecutevely.
 *
 *  E.g.
 *  \code
 *   std::tuple<int, double, QString> value;
 *   query.step(value);
 * \endcode
 * is equivalent to
 *  \code
 *   int x;
 *   double y;
 *   QString z;
 *   query.step(x, y, z);
 * \endcode
 *  @section fetchothertypes Other data types
 *  @subsection fetchunused Unused
 *  Passing an Unused<N> parameters will consume N columns without fetching any data. No error will be returned except for checking the number of fetched columns.
 *  \code
 *   int x, y;
 *   db->executeSingleAll("SELECT 1,2,3,4", x, Unused<2>, y); // x is 1, y is 4
 * \endcode
 *  @subsection fetchcall Call
 *  Passing a Call<T...>(f) parameter allows fetching data of type T... and call the corresponding function after the data is fetched.
 *  \see Call
 *  @subsection fetchblob Blob
 *  Passing a Blob to a column will behave differently depending on the column type.
 *  * If column is a blob then the Blob::setDatabase, Blob::table and Blob::setColumn will be called with the matching values of retrived column
 *  * If column is an integer Blob::setRowId will be called.
 *  Passing a reference to a Blob in a column that is not a blob or integer gives an error in strict mode and is a no-op in standard mode
 *  @section fetchcustomtypes Custom data types
 *  It is possible to handle the fetching of any data type T by implementing a function
 *
 *  void customFetch(CustomFetch &fetch, T &data)
 *
 *  This function will have to use the passed accessor to retrive the needed column(s) and set data
 *  \see CustomFetch
 */

  /** @page bindtypes Bounded data types
   *  This section describes the data types that are handled by the bounding functions (e.g. Query::bind, Db::executeSingle<N>, ...).
   *  Bounding function exists with a standard and a temporary version. They behave exactly the same except that the lifetime of QString and QByteArray bounded with temporary versions are guaranteed
   *  to extend until the Query will be destroyed or reset. They are mainly used for optimizing calls in executeSingle.
   *  @section bindnativedatatypes Native data types
   *  Following types are directly mapped to sqlite_bind_X calls
   *  \code
   *    query.bind(4, "Hello, world!");
   *  \endcode
   *  @subsection bindinttypes Integer types
   *  The following types are handled natively:
   *  - qint64
   *  All other integers can be used and will be simply cast to a qint64
   *
   *  Note: as SQLite stores only 64 bit signed integers the results of operations on 64 bit unsigned with the most signficant bit set is undefined
   *  @subsection bindfloattypes Floating point
   *  The following types are handled natively:
   *  - double
   *  A float can be used and will be simply cast to a double
   *  @subsection bindstringbytearray Other native types
   *  The following types are handled natively:
   *  - QString
   *  - QByteArray
   *
   *  Note that when using temporary version the passed variables must be guaranteed to exists for all the time they will be bound (that is until destruction of the query or a call to resetAllBindings)
   *  @section boundcpptypes C++ data types
   *  @subsection bindoptional std::optional<T>
   *  If the bound value is empty than this is equivalent to NULL, othewise the value will be bound as if directly passing a type T.
   *  \code
   *    std::optional<int> x(4),y;
   *    qry.bind(x,y); // 1st column is 4, 2nd is NULL
   *  \endcode
   *  @subsection tuple std::tuple<T...>
   *  If the bound data is a tuple then the data is retrived as if each element of the tuple was bound consecutevely.
   *
   *  E.g.
   *  \code
   *    query.bind(std::tuple<int, double, QString>(3, 5.2, "Hello world"));
   *  \endcode
   * is equivalent to
   *  \code
   *   int x;
   *   double y;
   *   QString z;
   *   query.bind(3, 5.2, "Hello world");
   * \endcode
   *  @section othertypes Other data types
   *  @subsection null Nulls
   *  Nulls can be bound in two ways:
   *  * By passing a nullptr
   *  * By passing a Null()
   *  \code
   *   // Both are equivalent and binds a NULL
   *   qry.bind(nullptr);
   *   qry.bind(Null());
   * \endcode
   *  @subsection zeroblob ZeroBlob
   *  Passing a ZeroBlob will bind an empty blob with the passed size. Mapped to a sqlite3_bind_zeroblob64 call.
   *  \code
   *   qry.bind(ZeroBlob(50));
   * \endcode
 *  @section bindcustomtypes Custom data types
 *  It is possible to handle the binding of any data type T by implementing one of the following functions:
 *
 *  * void customFetch(CustomBind &bind, const T &data)
 *  * void customFetch(CustomBind &bind, T &&data)
 *
 *  This function will have to use the passed accessor to bind the needed column(s) using data
 *  \see CustomBind
 */

  /** @page howtocompile Build instruction and requirements
   *  HFSQtLi requires Qt as well as SQLite. For more information on Qt please consult http://www.qt.io and for SQLite please see https://sqlite.org/.
   *  The library was tested with Qt 6.2.2 and SQLite 3.37.1. It does not provide the Qt library (which should be downloaded and installed separately) nor
   *  SQLite library which amalgamation should be downloaded and put in Test and/or HFSQtLi folders.
   *
   *  The library developement is done with the project found in Test. When the library is considered stable it is possible to run the script Test/amalgamate.py.
   *  This will parse the headers and source files and generate one header (HFSQtLi/HFSQtLi.h) and one header (HFSQtLi/HFSQtLi.cpp).
   *  Those files can be included in any external project to give access to the full functionality of the library.
   *
   *  The source should be compiled with SQLITE_ENABLE_COLUMN_METADATA to enable the automatic binding of blobs. Without it a warning will be issued when compiling. The library will
   *  still fully work except expression such as:
   * \code
   *   Blob b; // Note that auto-open is enabled by default
   *   m_db->executeSingle("SELECT blobColumn, rowid FROM TestTable WHERE name="Foo", b, b);
   * \endcode
   * Will not automatically set the Blob parameters
  */

}

namespace HFSQtLi
{
  /** @page license License
   *  Copyright 2021 Marzocchi Alessandro
   *
   *  Licensed under the Apache License, Version 2.0 (the "License");
   *  you may not use this file except in compliance with the License.
   *
   *  Unless required by applicable law or agreed to in writing, software
   *  distributed under the License is distributed on an "AS IS" BASIS,
   *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   *
   *  See the License for the specific language governing permissions and
   *  limitations under the License.
   *
   *  Apache License
   *  Version 2.0, January 2004
   *  http://www.apache.org/licenses/
   *
   * <b>TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION</b>
   *
   * 1. Definitions.
   *
   *   "License" shall mean the terms and conditions for use, reproduction, and distribution as defined by Sections 1 through 9 of this document.
   *
   *   "Licensor" shall mean the copyright owner or entity authorized by the copyright owner that is granting the License.
   *
   *   "Legal Entity" shall mean the union of the acting entity and all other entities that control, are controlled by, or are under common control with that entity. For the purposes of this definition, "control" means (i) the power, direct or indirect, to cause the direction or management of such entity, whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or more of the outstanding shares, or (iii) beneficial ownership of such entity.
   *
   *   "You" (or "Your") shall mean an individual or Legal Entity exercising permissions granted by this License.
   *
   *   "Source" form shall mean the preferred form for making modifications, including but not limited to software source code, documentation source, and configuration files.
   *
   *   "Object" form shall mean any form resulting from mechanical transformation or translation of a Source form, including but not limited to compiled object code, generated documentation, and conversions to other media types.
   *
   *   "Work" shall mean the work of authorship, whether in Source or Object form, made available under the License, as indicated by a copyright notice that is included in or attached to the work (an example is provided in the Appendix below).
   *
   *   "Derivative Works" shall mean any work, whether in Source or Object form, that is based on (or derived from) the Work and for which the editorial revisions, annotations, elaborations, or other modifications represent, as a whole, an original work of authorship. For the purposes of this License, Derivative Works shall not include works that remain separable from, or merely link (or bind by name) to the interfaces of, the Work and Derivative Works thereof.
   *
   *   "Contribution" shall mean any work of authorship, including the original version of the Work and any modifications or additions to that Work or Derivative Works thereof, that is intentionally submitted to Licensor for inclusion in the Work by the copyright owner or by an individual or Legal Entity authorized to submit on behalf of the copyright owner. For the purposes of this definition, "submitted" means any form of electronic, verbal, or written communication sent to the Licensor or its representatives, including but not limited to communication on electronic mailing lists, source code control systems, and issue tracking systems that are managed by, or on behalf of, the Licensor for the purpose of discussing and improving the Work, but excluding communication that is conspicuously marked or otherwise designated in writing by the copyright owner as "Not a Contribution."
   *
   *   "Contributor" shall mean Licensor and any individual or Legal Entity on behalf of whom a Contribution has been received by Licensor and subsequently incorporated within the Work.
   *
   * 2. Grant of Copyright License. Subject to the terms and conditions of this License, each Contributor hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable copyright license to reproduce, prepare Derivative Works of, publicly display, publicly perform, sublicense, and distribute the Work and such Derivative Works in Source or Object form.
   *
   * 3. Grant of Patent License. Subject to the terms and conditions of this License, each Contributor hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable (except as stated in this section) patent license to make, have made, use, offer to sell, sell, import, and otherwise transfer the Work, where such license applies only to those patent claims licensable by such Contributor that are necessarily infringed by their Contribution(s) alone or by combination of their Contribution(s) with the Work to which such Contribution(s) was submitted. If You institute patent litigation against any entity (including a cross-claim or counterclaim in a lawsuit) alleging that the Work or a Contribution incorporated within the Work constitutes direct or contributory patent infringement, then any patent licenses granted to You under this License for that Work shall terminate as of the date such litigation is filed.
   *
   * 4. Redistribution. You may reproduce and distribute copies of the Work or Derivative Works thereof in any medium, with or without modifications, and in Source or Object form, provided that You meet the following conditions:
   *   You must give any other recipients of the Work or Derivative Works a copy of this License; and
   *
   *   You must cause any modified files to carry prominent notices stating that You changed the files; and
   *
   *   You must retain, in the Source form of any Derivative Works that You distribute, all copyright, patent, trademark, and attribution notices from the Source form of the Work, excluding those notices that do not pertain to any part of the Derivative Works; and
   *
   *   If the Work includes a "NOTICE" text file as part of its distribution, then any Derivative Works that You distribute must include a readable copy of the attribution notices contained within such NOTICE file, excluding those notices that do not pertain to any part of the Derivative Works, in at least one of the following places: within a NOTICE text file distributed as part of the Derivative Works; within the Source form or documentation, if provided along with the Derivative Works; or, within a display generated by the Derivative Works, if and wherever such third-party notices normally appear. The contents of the NOTICE file are for informational purposes only and do not modify the License. You may add Your own attribution notices within Derivative Works that You distribute, alongside or as an addendum to the NOTICE text from the Work, provided that such additional attribution notices cannot be construed as modifying the License.
   *
   *   You may add Your own copyright statement to Your modifications and may provide additional or different license terms and conditions for use, reproduction, or distribution of Your modifications, or for any such Derivative Works as a whole, provided Your use, reproduction, and distribution of the Work otherwise complies with the conditions stated in this License.
   *
   * 5. Submission of Contributions. Unless You explicitly state otherwise, any Contribution intentionally submitted for inclusion in the Work by You to the Licensor shall be under the terms and conditions of this License, without any additional terms or conditions. Notwithstanding the above, nothing herein shall supersede or modify the terms of any separate license agreement you may have executed with Licensor regarding such Contributions.
   *
   * 6. Trademarks. This License does not grant permission to use the trade names, trademarks, service marks, or product names of the Licensor, except as required for reasonable and customary use in describing the origin of the Work and reproducing the content of the NOTICE file.
   *
   * 7. Disclaimer of Warranty. Unless required by applicable law or agreed to in writing, Licensor provides the Work (and each Contributor provides its Contributions) on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR PURPOSE. You are solely responsible for determining the appropriateness of using or redistributing the Work and assume any risks associated with Your exercise of permissions under this License.
   * 8. Limitation of Liability. In no event and under no legal theory, whether in tort (including negligence), contract, or otherwise, unless required by applicable law (such as deliberate and grossly negligent acts) or agreed to in writing, shall any Contributor be liable to You for damages, including any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or out of the use or inability to use the Work (including but not limited to damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses), even if such Contributor has been advised of the possibility of such damages.
   *
   * 9. Accepting Warranty or Additional Liability. While redistributing the Work or Derivative Works thereof, You may choose to offer, and charge a fee for, acceptance of support, warranty, indemnity, or other liability obligations and/or rights consistent with this License. However, in accepting such obligations, You may act only on Your own behalf and on Your sole responsibility, not on behalf of any other Contributor, and only if You agree to indemnify, defend, and hold each Contributor harmless for any liability incurred by, or claims asserted against, such Contributor by reason of your accepting any such warranty or additional liability.
   *
   * <b>END OF TERMS AND CONDITIONS</b>
  */
}
