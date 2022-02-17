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

#include "database.h"
#include "sqlite3.h"
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
