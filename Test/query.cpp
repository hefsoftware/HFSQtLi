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

#include "query.h"
#include "blob.h"
#include "sqlite3.h"

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
