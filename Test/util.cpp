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

#include "util.h"
#include "sqlite3.h"

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
