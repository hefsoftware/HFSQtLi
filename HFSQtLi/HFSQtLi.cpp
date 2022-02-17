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
#include "sqlite3.h"
#include "HFSQtLi.h"


Q_STATIC_ASSERT(SQLITE_OK==0);

using namespace HFSQtLi;
const int SQLiteCode::MISUSE=SQLITE_MISUSE;
const int SQLiteCode::CONSTRAINT=SQLITE_CONSTRAINT;
const int SQLiteCode::OK=SQLITE_OK;
const int SQLiteCode::DONE=SQLITE_DONE;

bool SQLiteCode::isSuccess(int code)
{
  switch(code)
  {
    case SQLITE_OK:
    case SQLITE_DONE:
    case SQLITE_ROW:
      return true;
    default:
      return false;
  }
}

QString SQLiteCode::errorStringFull(int code)
{
  return QString::fromUtf8(sqlite3_errstr(code));
}

QString SQLiteCode::errorString(int code)
{
  return isSuccess(code)?QString():QString::fromUtf8(sqlite3_errstr(code));
}

Type SQLiteCode::typeFromSqlite(int type)
{
    Type ret;
    switch(type)
    {
    case SQLITE_NULL: ret=Type::Null; break;
    case SQLITE_INTEGER: ret=Type::Integer; break;
    case SQLITE_FLOAT: ret=Type::Float; break;
    case SQLITE_TEXT: ret=Type::Text; break;
    case SQLITE_BLOB: ret=Type::Blob; break;
    default:
      ret=Type::Invalid;
    }
    return ret;
}


Value::Value(const Value &other): m_value(nullptr)
{
  if(other.m_value)
    m_value=sqlite3_value_dup(other.m_value);
}

void Value::clear()
{
  if(m_value)
    sqlite3_value_free(m_value);
  m_value=nullptr;
}

void Value::dupFrom(const Value &source)
{
  clear();
  if(source.m_value)
      m_value=sqlite3_value_dup(source.m_value);
}

double Value::toDouble()
{
  double ret=qQNaN();
  if(m_value)
    ret=sqlite3_value_double(m_value);
  return ret;
}

qint64 Value::toInt64()
{
  qint64 ret=0;
  if(m_value)
    ret=sqlite3_value_int64(m_value);
  return ret;
}

QString Value::toString()
{
  QString ret;
  if(m_value)
    ret=QString::fromUtf8(sqlite3_value_text(m_value));
  return ret;
}

bool Value::isNull()
{
  return m_value&& (sqlite3_value_type(m_value)==SQLITE_NULL);
}

bool Value::isInt()
{
  return m_value&& (sqlite3_value_type(m_value)==SQLITE_INTEGER);
}

bool Value::isFloat()
{
  return m_value&& (sqlite3_value_type(m_value)==SQLITE_FLOAT);
}

bool Value::isText()
{
  return m_value&& (sqlite3_value_type(m_value)==SQLITE_TEXT);
}

bool Value::isBlob()
{
  return m_value&& (sqlite3_value_type(m_value)==SQLITE_BLOB);
}

Type Value::type()
{
  return m_value?SQLiteCode::typeFromSqlite(sqlite3_value_type(m_value)):Type::Invalid;
}


#ifndef SQLITE_ENABLE_COLUMN_METADATA
#warning SQLITE_ENABLE_COLUMN_METADATA not enabled. Reduced BLOB functionality (see documentation in section "How to compile")
#endif

using namespace HFSQtLi;
Query::Query(Db *db, const char *query, bool persistent, bool storeErrorMsg, const char **tail): m_db(db), m_stmt(nullptr), m_keepErrorMsg(storeErrorMsg)
{
  if(db)
    db->m_queryCount++;
  prepare(query, persistent, tail);
}

Query::Query(Db *db, const QString &query, bool persistent, bool storeErrorMsg, QString *tail): m_db(db), m_stmt(nullptr), m_keepErrorMsg(storeErrorMsg)
{
  if(db)
    db->m_queryCount++;
  prepare(query, persistent, tail);
}

Query::Query(Db *db, bool storeErrorMsg): m_db(db), m_stmt(nullptr), m_keepErrorMsg(storeErrorMsg)
{
  if(db)
    db->m_queryCount++;
}

Query::~Query()
{
  finalize();
  if(m_db)
    m_db->m_queryCount--;
}

bool Query::prepare(const char *query, bool persistent, const char **tail)
{
  if(isDbValid())
  {
    Db::Lock lock(m_db, m_keepErrorMsg);
    m_error=SQLITE_OK;
    if(m_stmt)
    {
      m_error=sqlite3_finalize(m_stmt);
      m_stmt=nullptr;
    }
    if(m_error==SQLITE_OK)
      m_error=sqlite3_prepare_v3(m_db->m_db, query?query:"", -1, persistent?SQLITE_PREPARE_PERSISTENT:0, &m_stmt, tail);
    lock.release(m_errorMsg);
  }
  else
  {
    setInternalError(SQLITE_MISUSE);
  }
  return (m_error==SQLITE_OK);
}

bool Query::prepare(const QString &query, bool persistent, QString *tail)
{
  if(isDbValid())
  {
    const void *tailPtr=nullptr;
    Db::Lock lock(m_db, m_keepErrorMsg);
    m_error=SQLITE_OK;
    if(m_stmt)
    {
      m_error=sqlite3_finalize(m_stmt);
      m_stmt=nullptr;
    }
    if(m_error==SQLITE_OK)
      m_error=sqlite3_prepare16_v3(m_db->m_db, query.data(), query.size()*sizeof(QChar), persistent?SQLITE_PREPARE_PERSISTENT:0, &m_stmt, &tailPtr);
    lock.release(m_errorMsg);
    if(m_error==SQLITE_OK && tail)
      *tail=QString::fromUtf16((char16_t *)tailPtr);
  }
  else
  {
    setInternalError(SQLITE_MISUSE);
  }
  return (m_error==SQLITE_OK);
}

bool Query::stepNoFetch()
{
  int ret=0;
  if(!isPrepared())
    setInternalError(SQLITE_MISUSE);
  else
  {
    Db::Lock lock(m_db, m_keepErrorMsg);
    m_error=sqlite3_step(m_stmt);
    lock.release(m_errorMsg);
    ret=(m_error==SQLITE_ROW)?1:0;
  }
  return ret;
}

bool Query::finalize()
{
  bool ret=false;
  if(!isPrepared())
    setInternalError(SQLITE_MISUSE);
  else
  {
    Db::Lock lock(m_db, m_keepErrorMsg);
    m_error=sqlite3_finalize(m_stmt);
    lock.release(m_errorMsg);
    m_stmt=nullptr;
    ret=(m_error==SQLITE_OK);
  }
  return ret;
}

bool Query::reset()
{
    bool ret=false;
    if(!isPrepared())
      setInternalError(SQLITE_MISUSE);
    else
    {
      Db::Lock lock(m_db, m_keepErrorMsg);
      m_error=sqlite3_reset(m_stmt);
      lock.release(m_errorMsg);
      ret=(m_error==SQLITE_OK);
    }
    return ret;
}

bool Query::clearBindings()
{
  bool ret=false;
  if(!isPrepared())
    setInternalError(SQLITE_MISUSE);
  else
  {
    Db::Lock lock(m_db, m_keepErrorMsg);
    m_error=sqlite3_clear_bindings(m_stmt);
    lock.release(m_errorMsg);
    ret=(m_error==SQLITE_OK);
  }
  return ret;
}


int Query::bindSingle(bool, int i, qint64 value)
{
  m_error=sqlite3_bind_int64(m_stmt, i, value);
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool, int i, double value)
{
  m_error=sqlite3_bind_double(m_stmt, i, value);
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool, int i, std::nullptr_t)
{
  m_error=sqlite3_bind_null(m_stmt, i);
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool, int i, const ZeroBlob &zeroBlob)
{
  m_error=sqlite3_bind_zeroblob64(m_stmt, i, zeroBlob.size());
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool, int i, const char *value, int size)
{
//  m_error=sqlite3_bind_text(m_stmt, i, value.toUtf8(), -1, SQLITE_TRANSIENT);
  m_error=sqlite3_bind_text(m_stmt, i, value, size, SQLITE_TRANSIENT);
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool temporary, int i, const QString &value)
{
  m_error=sqlite3_bind_text16(m_stmt, i, value.data(), -1, temporary?SQLITE_STATIC: SQLITE_TRANSIENT);
  return fetchErrorString()?2:0;
}

int Query::bindSingle(bool temporary, int i, const QByteArray &value)
{
  m_error=sqlite3_bind_blob64(m_stmt, i, value.data(), value.size(), temporary?SQLITE_STATIC: SQLITE_TRANSIENT);
  return fetchErrorString()?2:0;
}

int Query::error(QString *errorOut)
{
  if(errorOut)
    *errorOut=errorMsg();
  return m_error;
}

bool Query::isOk()
{
  return (m_error==SQLITE_OK);
}

bool Query::isDone()
{
  return (m_error==SQLITE_DONE);
}

void Query::setKeepErrorMsg(bool newKeepErrorMsg)
{
  if(!m_keepErrorMsg && newKeepErrorMsg)
    m_errorMsg=errorMsg(); // Store the coded error message
  m_keepErrorMsg = newKeepErrorMsg;
}

int Query::clearBindingInternal()
{
  int ret=SQLITE_MISUSE;
  if(m_stmt)
    ret=sqlite3_clear_bindings(m_stmt);
  return ret;
}

void Query::resetInternalError()
{
  m_error=SQLITE_OK;
  m_errorMsg.clear();
}

QString Query::errorMsg() const
{
  if(m_keepErrorMsg)
    return m_errorMsg;
  else
    return SQLiteCode::errorString(m_error);
}

void Query::setInternalError(int code, const char *explicitMessage)
{
  m_error=code;
  if(m_keepErrorMsg)
  {
    if(explicitMessage)
      m_errorMsg=QString::fromUtf8(explicitMessage);
    else if(SQLiteCode::isSuccess(code))
      m_errorMsg.clear();
    else
      m_errorMsg=SQLiteCode::errorString(code);
  }
}

bool Query::keepErrorMsg() const
{
    return m_keepErrorMsg;
}

void Query::forceFetchErrorString()
{
  if(m_error==SQLITE_MISUSE)
    m_errorMsg=SQLiteCode::errorString(SQLITE_MISUSE);
  else
    m_errorMsg=QString::fromUtf8(sqlite3_errmsg(db()->internalDb()));
}

int Query::assertFetchColumnCount(int i)
{
  int ret=-1;
  Q_ASSERT(m_stmt);
  int numCol=sqlite3_column_count(m_stmt);
  if(i==numCol)
    ret=i;
  else
  {
    setInternalError(SQLITE_CONSTRAINT, QString("Fetched %1 values in a query returning %2").arg(i).arg(numCol).toUtf8());
  }
  return ret;
}

int Query::assertBindColumnCount(int i)
{
  int ret=-1;
  if(!m_stmt)
    setInternalError(SQLITE_MISUSE);
  else
  {
    int numCol=sqlite3_bind_parameter_count(m_stmt);
    if(i==numCol)
      ret=i;
    else
      setInternalError(SQLITE_CONSTRAINT, QString("Bound %1 value(s) in a query using %2").arg(i).arg(numCol).toUtf8());
  }
  return ret;
}

Type Query::columnType(int i)
{
  Type ret=Type::Invalid;
  if(!m_stmt)
    setInternalError(SQLITE_MISUSE);
  else
  {
    ret=SQLiteCode::typeFromSqlite(sqlite3_column_type(m_stmt, i));
    if(ret==Type::Invalid)
      setInternalError(SQLITE_MISUSE, "Unknown type returned from sqlite3_column_type");
  }
  return ret;
}

qint64 Query::readColumnIntSQLite(int i, bool &ok)
{
  ok=true;
  return sqlite3_column_int64(m_stmt, i);
}

double Query::readColumnDoubleSQLite(int i, bool &ok)
{
  ok=true;
  return sqlite3_column_double(m_stmt, i);
}

int Query::readColumn(bool strict, int i, QString &value)
{
  bool ok=true;
  if(strict)
  {
    if(columnType(i)!=Type::Text)
    {
      setInternalError(SQLiteCode::CONSTRAINT, "Read column was not a string");
      ok=false;
    }
    else
      value=QString::fromUtf8(sqlite3_column_text(m_stmt, i));
  }
  else
    value=QString::fromUtf8(sqlite3_column_text(m_stmt, i));
  return ok?2:0;
}

int Query::readColumn(bool, int i, Value &result)
{
  int ret=0;
  result.clear();
  if(!isPrepared())
    setInternalError(SQLITE_MISUSE);
  else
  {
    Db::Lock lock(m_db, m_keepErrorMsg);
    auto value=sqlite3_column_value(m_stmt, i); // Note: not sure if the value should be freed. However calling free on it results in crash so maybe not.
    if(!value)
      lock.release(m_errorMsg);
    else
    {
      result=Value(sqlite3_value_dup(value));
      setInternalError(result.isValid()?SQLITE_OK:SQLITE_NOMEM);
      lock.release();
    }
    ret=(m_error==SQLITE_OK)?2:0;
  }
  return ret;
}

int Query::readColumn(bool strict, int i, Blob &value)
{
  return readColumnInternal(i, value, strict);
}

int Query::readColumn(bool strict, int i, QByteArray &value)
{
  bool ok=false;
  if(strict && columnType(i)!=Type::Blob)
  {
    setInternalError(SQLiteCode::CONSTRAINT, "Read column was not a blob");
  }
  else
  {
    int bytes=sqlite3_column_bytes(m_stmt, i);
    if(bytes>0)
    {
      value.resize(bytes);
      memcpy(value.data(), sqlite3_column_blob(m_stmt, i), bytes);
      ok=true;
    }
    else if(bytes==0)
    {
      value.clear();
      ok=true;
    }
  }
  return ok?2:0;

}

int Query::readColumnInternal(int i, Blob &value, bool strict)
{
  if(!m_stmt)
    setInternalError(SQLITE_MISUSE);
  else
  {
    int type=sqlite3_column_type(m_stmt, i);
    if(type!=SQLITE_BLOB && type!=SQLITE_INTEGER)
    {
      if(strict)
        setInternalError(SQLITE_MISMATCH, "Expected a blob or integer column when fetching a Blob type");
      else
        setInternalError(SQLITE_OK);
    }
    else if(type==SQLITE_BLOB)/* if((m_error=value.close(&m_errorMsg))==SQLITE_OK)*/
    {
      Db::Lock lock(m_db, m_keepErrorMsg);
#ifdef SQLITE_ENABLE_COLUMN_METADATA
      value.set(
        m_db,
        sqlite3_column_database_name(m_stmt, i),
        sqlite3_column_table_name(m_stmt, i),
        sqlite3_column_origin_name(m_stmt, i)
      );
#else
      value.set(
        m_db,
        value.database().toUtf8().data(),
        value.table().toUtf8().data(),
        value.column().toUtf8().data()
      );
#endif
      lock.release(m_db->error(), m_errorMsg);
    }
    else if(type==SQLITE_INTEGER)
    {
      Db::Lock lock(m_db, m_keepErrorMsg);
      value.setRowId(sqlite3_column_int64(m_stmt, i));
      lock.release(m_db->error(), m_errorMsg);
    }
  }
  return (m_error==SQLITE_OK)?2:0;
}

using namespace HFSQtLi;

Db *Db::open(const QString &filename, QIODevice::OpenMode flags, QString *errorMsg, const char *zVfs)
{
  Db *ret=new Db(filename,flags,zVfs);
  if(errorMsg)
    *errorMsg=ret->errorMsg();
  if(!ret->isOk())
  {
    delete ret;
    ret=nullptr;
  }
  return ret;
}

Db::Db(const QString &filename, QIODevice::OpenMode flags, const char *zVfs)
{
  int sqliteFlags=SQLITE_OPEN_EXRESCODE;
  if(flags&QIODevice::ReadWrite)
    sqliteFlags|=SQLITE_OPEN_READWRITE;
  else
    sqliteFlags|=SQLITE_OPEN_READONLY;
  if(!(flags&QIODevice::Append))
    sqliteFlags|=SQLITE_OPEN_CREATE;

  m_openError=sqlite3_open_v2(filename.toUtf8(),
                          &m_db,
                          sqliteFlags,
                          zVfs);
  m_queryCount=0;
  if(!m_db)
    m_openErrorMsg=SQLiteCode::errorString(m_openError);
}

Db::~Db()
{
  Q_ASSERT(m_queryCount==0);
  if(m_db)
    sqlite3_close_v2(m_db);
}

bool Db::isOk() const
{
  return (error()==SQLITE_OK);
}


int Db::error() const
{
  return m_db?sqlite3_errcode(m_db):m_openError;
}

QString Db::errorMsg() const
{
  return m_db?QString::fromUtf8(sqlite3_errmsg(m_db)):m_openErrorMsg;
}

Query *Db::query(const char *queryStr, bool persistent, bool keepErrorMessage)
{
  auto ret=new Query(this, queryStr, persistent, keepErrorMessage);
  return ret;
}

Db::Lock::Lock(Db *db, bool lock)
{
  Q_ASSERT(db);
  if(lock && db && db->m_db)
  {
    m_db=db->m_db;
    sqlite3_mutex_enter(sqlite3_db_mutex(m_db));
  }
  else
    m_db=nullptr;
}

Db::Lock::~Lock()
{
  release();
}

void Db::Lock::release()
{
  if(m_db)
    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));
  m_db=nullptr;
}

void Db::Lock::release(QString &msg)
{
  if(m_db)
    release(sqlite3_errcode(m_db), msg);
//  else
//    qWarning()<<"release(QString) on unset lock";
}

void Db::Lock::release(int code, QString &msg)
{
  if(m_db)
  {
    if(SQLiteCode::isSuccess(code))
      msg.clear();
    else if(code==SQLITE_MISUSE)
      msg=SQLiteCode::errorString(SQLITE_MISUSE);
    else
      msg=QString::fromUtf8(sqlite3_errmsg(m_db));

    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));
    m_db=nullptr;
  }
  else
    qWarning("release(int, QString) on unset lock");
}

int Db::Lock::releaseInternal(int code, QString &msg)
{
  if(m_db)
  {
    if(SQLiteCode::isSuccess(code))
      msg.clear();
    else
      msg=SQLiteCode::errorString(code);

    sqlite3_mutex_leave(sqlite3_db_mutex(m_db));
    m_db=nullptr;
  }
  else
    qWarning("releaseInternal on unset lock");
  return code;
}

int Db::Lock::releaseInternal(int code, QString &msg, const char *explicitMessage)
{
    if(m_db)
    {
      msg=QString::fromUtf8(explicitMessage);
      sqlite3_mutex_leave(sqlite3_db_mutex(m_db));
      m_db=nullptr;
    }
    else
      qWarning("releaseInternal on unset lock");
    return code;
}


using namespace HFSQtLi;
using namespace HFSQtLi::Helper;

qint64 BlobData::size()
{
  qint64 ret=-1;
  if(m_blob)
  {
    if(m_sizeCache<=0)
      m_sizeCache=sqlite3_blob_bytes(m_blob);
    ret=m_sizeCache;
  }
  return ret;
}

bool BlobData::close(QString *errorMsg)
{
  int ret=SQLITE_OK;
  if(m_blob)
  {
    Db::Lock lock(m_db, true);
    if((ret=sqlite3_blob_close(m_blob))==SQLITE_OK)
    {
      m_blob=nullptr;
      m_sizeCache=-1;
    }
    lock.release(errorMsg);
  }
  return (ret==SQLITE_OK);
}

bool BlobData::open(QString *errorMsg)
{
  int ret=SQLITE_MISUSE;
  if(m_db && m_table && m_column && m_hasRowId && !m_blob)
  {
    Db::Lock lock(m_db, true);
    ret=sqlite3_blob_open(m_db->m_db, m_database?m_database:"main", m_table, m_column, m_rowId, m_readWrite?1:0, &m_blob);
    lock.release(errorMsg);
  }
  else if(errorMsg)
    *errorMsg=SQLiteCode::errorString(SQLITE_MISUSE);
  return (ret==SQLITE_OK);
}

bool BlobData::open(qint64 id, QString *errorMsg)
{
  bool ret;
  if(isOpen())
    ret=reopenFast(id, errorMsg);
  else
  {
    m_rowId=id;
    m_hasRowId=true;
    ret=open(errorMsg);
  }
  return ret;
}

bool BlobData::reopenFast(qint64 newId, QString *errorMsg)
{
  int ret=SQLITE_MISUSE;
  if(m_blob)
  {
    m_rowId=newId;
    Db::Lock lock(m_db, true);
    ret=sqlite3_blob_reopen(m_blob, m_rowId);
    if(ret!=SQLITE_OK)
    {
      sqlite3_blob_close(m_blob);
      m_blob=nullptr;
    }
    lock.release(errorMsg);
  }
  else if(errorMsg)
    *errorMsg=SQLiteCode::errorString(SQLITE_MISUSE);
  m_sizeCache=-1;
  return (ret==SQLITE_OK);
}

BlobData::BlobData(Db *connection, const char *database, const char *table, const char *column, bool readWrite, bool autoOpen): m_db(connection), m_blob(nullptr), m_database(nullptr), m_table(nullptr), m_column(nullptr), m_rowId(std::numeric_limits<qint64>::min()), m_hasRowId(false), m_readWrite(readWrite), m_autoOpen(autoOpen), m_sizeCache(-1)
{
  setDatabase(database);
  setTable(table);
  setColumn(column);
}

BlobData::~BlobData()
{
  close(nullptr);
  if(m_db)
    m_db->m_queryCount--;
}

bool BlobData::read(void *data, qsizetype size, qsizetype offset)
{
  bool ret=false;
  if(m_blob)
    ret=(sqlite3_blob_read(m_blob, data, size, offset)==SQLITE_OK);
  return ret;
}

bool BlobData::write(const void *data, qsizetype size, qsizetype offset)
{
  bool ret=false;
  if(m_blob)
    ret=(sqlite3_blob_write(m_blob, data, size, offset)==SQLITE_OK);
  return ret;
}


bool BlobData::setDbPointer(Db *db)
{
  bool ret=false;
  if(db==m_db && (m_blob || !m_autoOpen)) // Not changing value on an already opened or a not auto-opening blob. No need to try to open it again.
    ret=true;
  else
  {
    if(close(nullptr))
    {
      if(m_db)
        m_db->m_queryCount--;
      m_db=db;
      if(m_db)
        m_db->m_queryCount++;
      ret=checkAutoOpen();
    }
  }
  return ret;
}

bool BlobData::setDatabase(const char *database)
{
  bool ret=false;
  if(((m_database && database && !strcmp(database, m_database)) || (!m_database && database)) && (m_blob || !m_autoOpen)) // Not changing value on an already opened or a not auto-opening blob. No need to try to open it again.
    ret=true;
  else
  {
    if(close(nullptr))
    {
      free(m_database);
      m_database=database?strdup(database):nullptr;
      ret=checkAutoOpen();
    }
  }
  return ret;
}


bool BlobData::setTable(const char *table)
{
  bool ret=false;
  if(m_table && table && !strcmp(table, m_table) && (m_blob || !m_autoOpen)) // Not changing value on an already opened or a not auto-opening blob. No need to try to open it again.
    ret=true;
  else
  {
    if(close(nullptr))
    {
      free(m_table);
      m_table=table?strdup(table):nullptr;
      ret=checkAutoOpen();
    }
  }
  return ret;
}

bool BlobData::setColumn(const char *column)
{
  bool ret=false;
  if(m_column && column && !strcmp(column, m_column) && (m_blob || !m_autoOpen)) // Not changing value on an already opened or a not auto-opening blob. No need to try to open it again.
    ret=true;
  else
  {
    if(close(nullptr))
    {
      free(m_column);
      m_column=column?strdup(column):nullptr;
      ret=checkAutoOpen();
    }
  }
  return ret;
}

bool BlobData::setRowId(qint64 value)
{
  bool ret=false;
  if(m_hasRowId && m_rowId==value && (m_blob || !m_autoOpen)) // Not changing value on an already opened or a not auto-opening blob. No need to try to open it again.
    ret=true;
  else
  {
    if(m_blob && m_autoOpen)
      ret=reopenFast(value);
    else if(close(nullptr))
    {
      m_rowId=value;
      m_hasRowId=true;
      if(!m_autoOpen || !isValid())
        ret=true;
      else
        ret=open();
    }
  }
  return ret;
}

bool BlobData::checkAutoOpen()
{
  bool ret=true;
  if(m_autoOpen && m_db && m_table && m_column && m_hasRowId)
    ret=open(nullptr);
  return ret;
}

bool Blob::set(Db *db, const char *database, const char *table, const char *column, qint64 rowid)
{
  bool curAutoOpen=m_data->autoOpen();
  bool ret;
  ret=m_data->setAutoOpen(false) && m_data->setDbPointer(db) && m_data->setDatabase(database) && m_data->setTable(table) && m_data->setColumn(column) && m_data->setRowId(rowid);
  m_data->setAutoOpen(curAutoOpen);
  ret&=m_data->checkAutoOpen();
  return ret;
}

bool Blob::set(Db *db, const char *database, const char *table, const char *column)
{
  bool curAutoOpen=m_data->autoOpen();
  bool ret;
  ret=m_data->setAutoOpen(false) && m_data->setDbPointer(db) && m_data->setDatabase(database) && m_data->setTable(table) && m_data->setColumn(column);
  m_data->setAutoOpen(curAutoOpen);
  ret&=m_data->checkAutoOpen();
  return ret;
}

bool Blob::set(Db *db, const char *database, const char *table, qint64 rowid)
{
  bool curAutoOpen=m_data->autoOpen();
  bool ret;
  ret=m_data->setAutoOpen(false) && m_data->setDbPointer(db) && m_data->setDatabase(database) && m_data->setTable(table) && m_data->setRowId(rowid);
  m_data->setAutoOpen(curAutoOpen);
  ret&=m_data->checkAutoOpen();
  return ret;
}
