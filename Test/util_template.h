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
#include "util.h"
#include "query.h"

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
