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
#include <string>
#include <typeinfo>
#include <iostream>
#include <vector>
#include <tuple>
#include <functional>
/// \cond INTERNAL

namespace NameType
{
template <typename T> struct NameOfBaseType
{
  static std::string name()
  {
    return typeid(T).name();
  }
};
template <> struct NameOfBaseType<void> { static std::string name() { return "void"; } };
template <> struct NameOfBaseType<int> { static std::string name() { return "int"; } };
template <> struct NameOfBaseType<unsigned> { static std::string name() { return "unsigned"; } };
template <> struct NameOfBaseType<double> { static std::string name() { return "double"; } };
template <> struct NameOfBaseType<float> { static std::string name() { return "float"; } };
template <> struct NameOfBaseType<bool> { static std::string name() { return "bool"; } };

template <typename T> struct CVRName
{
  typedef std::remove_cv_t<T> reduced;
  static std::string name()
  {
    std::string ret;
    if(std::is_volatile<T>())
      ret+="volatile ";
    if(std::is_const<T>())
      ret+="const ";
    return ret;
  }
};

inline std::string replace(const std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    std::string ret(subject);
    while ((pos = ret.find(search, pos)) != std::string::npos) {
         ret.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return ret;
}

struct NameBase
{
  std::string name()
  {
    std::string ret=prefix()+suffix();
    ret=replace(ret, "OPEN_FUNC_PARENT()CLOSE_FUNC_PARENT", "");
    ret=replace(ret, "OPEN_FUNC_PARENT(", "(");
    ret=replace(ret, ")CLOSE_FUNC_PARENT", ")");
    ret=replace(ret, ")", ")");
    return ret;
  }
  virtual std::string prefix()=0;
  virtual std::string suffix() { return ""; }
};

/* Functions to print a data type */
template <typename T> struct Name: public NameBase
{
  std::string prefix() override
  {
    typedef typename CVRName<T>::reduced S;
    std::string ret;
    if(std::is_same<T, S>())
      ret+=NameOfBaseType<T>::name()+" ";
    else
      ret+=Name<S>().prefix();
    ret+=CVRName<T>::name();
    return ret;
  }
  std::string suffix() override {
    typedef typename CVRName<T>::reduced S;
    if(std::is_same<T, S>())
      return "";
    else
      return Name<typename CVRName<T>::reduced>().suffix();
  }
};

template <typename T> struct Name<T *>: public NameBase
{
    std::string prefix() override
    {
      typedef typename CVRName<T>::reduced S;
      std::string ret;
      ret+=Name<S>().prefix();
      ret+=CVRName<T>::name();
      ret+="* ";
      return ret;
    }
    std::string suffix() override { return Name<typename CVRName<T>::reduced>().suffix(); }
};
template <typename T> struct Name<T &>: public NameBase
{
    std::string prefix() override
    {
      typedef typename CVRName<T>::reduced S;
      std::string ret;
      ret+=Name<S>().prefix();
      ret+=CVRName<T>::name();
      ret+="& ";
      return ret;
    }
    std::string suffix() override { return Name<typename CVRName<T>::reduced>().suffix(); }
};
template <typename T> struct Name<T &&>: public NameBase
{
    std::string prefix() override
    {
      typedef typename CVRName<T>::reduced S;
      std::string ret;
      ret+=Name<S>().prefix();
      ret+=CVRName<T>::name();
      ret+="&& ";
      return ret;
    }
    std::string suffix() override { return Name<typename CVRName<T>::reduced>().suffix(); }
};
template <typename T> struct Name<T []>: public NameBase
{
    std::string prefix() override
    {
      typedef typename CVRName<T>::reduced S;
      std::string ret,cvr(CVRName<T>::name());
      ret+=Name<S>().prefix();
      ret+=CVRName<T>::name();
      return ret;
    }
    std::string suffix() override
    {
      return Name<typename CVRName<T>::reduced>().suffix()+ "[]";
    }
};

inline std::string join(const std::vector<std::string> &l, const char *sep=", ")
{
    std::string ret;
    for(std::size_t i=0;i<l.size();i++)
    {
        if(i)ret+=sep;
        ret+=l[i];
    }
    return ret;
}
inline std::string joinParenthized(const std::vector<std::string> &l, const char *sep=", ")
{
    std::string ret("(");
    ret+=join(l, sep);
    ret+=")";
    return ret;
}
template <typename ...T> struct TypeNamePack;
template <typename S, typename ...T> struct TypeNamePack<S, T...>
{
    static void namesFill(std::vector<std::string> &out) { out.push_back(Name<S>().name()); TypeNamePack<T...>::namesFill(out); }
    static std::string name() { std::vector<std::string> l; namesFill(l); return join(l); }
};
template <> struct TypeNamePack< >
{
  static void namesFill(std::vector<std::string> &) { }
  static std::string name() { return ""; }
};

template <typename ...T> struct Name<std::tuple<T...>>: public NameBase
{
    std::string prefix() override
    {
      std::string ret;
      ret+="std::tuple<";
      ret+=TypeNamePack<T...>::name();
      ret+="> ";
      return ret;
    }
};

template <typename S, typename ...T> struct Name<std::function<S (T...)>>: public NameBase
{
    std::string prefix() override
    {
      std::string ret;
      typedef typename CVRName<S>::reduced R;
      ret+="std::function<";
      ret+=Name<R>().name();
      ret+=CVRName<S>::name();
      ret+="OPEN_FUNC_PARENT(";
      return ret;
    }
    std::string suffix() override
    {
      std::string ret;
      ret+=")CLOSE_FUNC_PARENT(";
      ret+=TypeNamePack<T...>::name();
      ret+=")";
      ret+=">";
      return ret;
    }
};
template <typename S, typename ...T> struct Name<S (T...)>: public NameBase
{
    std::string prefix() override
    {
      std::string ret;
      typedef typename CVRName<S>::reduced R;
      ret+=Name<R>().name();
      ret+=CVRName<S>::name();
      ret+="OPEN_FUNC_PARENT(";
      return ret;
    }
    std::string suffix() override
    {
      std::string ret;
      ret+=")CLOSE_FUNC_PARENT(";
      ret+=TypeNamePack<T...>::name();
      ret+=")";
      return ret;
    }
};

template <class T> std::string name()
{
  return Name<T>().name();
}
template <class ...T> std::string packName()
{
  return TypeNamePack<T...>::name();
}

}
/// \endcond INTERNAL
