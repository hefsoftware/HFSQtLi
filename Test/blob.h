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
#include <QSharedData>
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
