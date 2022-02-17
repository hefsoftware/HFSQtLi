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
