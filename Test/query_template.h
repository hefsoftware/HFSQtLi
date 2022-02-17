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
#include "query.h"
#include "database.h"
#include "util.h"
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
