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
#include "database.h"
#include "query.h"
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
