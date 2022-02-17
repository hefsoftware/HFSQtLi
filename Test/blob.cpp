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

#include "blob.h"
#include "database.h"
#include "sqlite3.h"

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
