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
#include <utility>
#include <functional>
#include <tuple>
namespace HFSQtLi
{
  /// \cond INTERNAL
  namespace Helper
  {

    template <int ...Index> using int_sequence=std::integer_sequence<int, Index...> ;
    template<int OFFSET, int ...INDEX> constexpr auto make_int_sequenceHelper (int_sequence<INDEX...> ) { return int_sequence<(INDEX+OFFSET)...>(); }
    /// @brief constructs an integer sequence with indexes START, START+1, ..., START+SIZE-1
    template<int START, int SIZE> constexpr auto make_int_sequence () { static_assert(SIZE>=0, "Size of make_int sequence must be >0"); return make_int_sequenceHelper<START>(std::make_integer_sequence<int, SIZE>()); }
    /// @brief constructs an integer sequence with indexes 0, 1, ..., SIZE-1
    template<int SIZE> constexpr auto make_int_sequence () { static_assert(SIZE>=0, "Size of make_int sequence must be >0"); return std::make_integer_sequence<int, SIZE>(); }

    template <typename T> T & forwardSingleAsRef(T & value) { return value;};
    template <typename T> const T &forwardSingleAsRef(T && value) {return value;};
    template <typename T> const T & forwardSingleAsRef(const T & value) {return value;};
    template <typename T> const T & forwardSingleAsRef(const T && value) {return value;};
    ///@ Returned [i] will be of type T& if args[i] is a left reference, const T &otherwise
    template <typename ...T> constexpr auto forwardAsRef(T &&...arg)
    {
      return std::forward_as_tuple(forwardSingleAsRef(std::forward<T>(arg))...);
    }

//    template <typename ...T, int... Index> constexpr auto tupleToRef(const std::tuple<T...> &values, int_sequence<Index...>)
//    {
//      return std::forward_as_tuple(forwardSingleAsRef(std::forward<T>(std::get<Index>(values)))...);
//    }
//    template <typename ...T> constexpr auto tupleToRef(const std::tuple<T...> &values)
//    {
//      return tupleToRef(values, make_int_sequence<sizeof...(T)>());
//    }

    template <typename T> struct MyForwardTupleRefT { typedef T &type; };
    template <typename T> struct MyForwardTupleRefT<T&> { typedef T &type; };
    template <typename T> struct MyForwardTupleRefT<T&&> { typedef T &&type; };
    template <typename T> struct MyForwardTupleRefT<const T&&> { typedef const T &&type; };
    ///@ Returned type will be of type T& if args[i] is a value, the original type otherwise
    template <typename T> using MyForwardTupleRef = typename MyForwardTupleRefT<T>::type;


    template<typename ...T , int ...Index> auto tuple_forwardHelper(std::tuple<T...> &value, Helper::int_sequence<Index...>)
    {
      return std::tuple<MyForwardTupleRef<T>...>((MyForwardTupleRef<T>)std::get<Index>(value)...);
    }

    ///@ Returned tuple[i] will be a left reference to value[i] if value[i] is a value, the original reference otherwise
    template<typename ...T> auto tuple_forward(std::tuple<T...> &value)
    {
      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
    }

    template <typename T> struct MyForwardConstTupleRefT { typedef const T &type; };
    template <typename T> struct MyForwardConstTupleRefT<T&> { typedef T &type; };
    template <typename T> struct MyForwardConstTupleRefT<T&&> { typedef T &&type; };
    template <typename T> struct MyForwardConstTupleRefT<const T&&> { typedef const T &&type; };
    template <typename T> using MyForwardConstTupleRef = typename MyForwardConstTupleRefT<T>::type;
    template<typename ...T , int ...Index> auto tuple_forwardHelper(const std::tuple<T...> &value, Helper::int_sequence<Index...>)
    {
      return std::tuple<MyForwardConstTupleRef<T>...>(static_cast<MyForwardConstTupleRef<T>>(std::get<Index>(value))...);
    }
    template<typename ...T> auto tuple_forward(const std::tuple<T...> &value)
    {
      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
    }
//    template <typename T> struct MyForwardRRefTupleRefT { typedef T &&type; };
//    template <typename T> struct MyForwardRRefTupleRefT<T&> { typedef T &type; };
//    template <typename T> struct MyForwardRRefTupleRefT<T&&> { typedef T &&type; };
//    template <typename T> struct MyForwardRRefTupleRefT<const T&&> { typedef const T &&type; };
//    template <typename T> using MyForwardRRefTupleRef = typename MyForwardRRefTupleRefT<T>::type;

//    template<typename ...T , int ...Index> auto tuple_forwardHelper(const std::tuple<T...> &&value, Helper::int_sequence<Index...>)
//    {
//      return std::tuple<MyForwardRRefTupleRef<T>...>(static_cast<MyForwardRRefTupleRef<T>>(std::get<Index>(value))...);
//    }
//    template<typename ...T> auto tuple_forward(const std::tuple<T...> &&value)
//    {
//      return tuple_forwardHelper(value, Helper::make_int_sequence<sizeof...(T)>());
//    }

    /// @brief Given a tuple returns a tuple created with the given subset of Indexes of the tuple
    template <typename ...T, int... Index> constexpr auto select(const std::tuple<T...> &tuple, Helper::int_sequence<Index...>)
    {
      using sourceTupleUnref=std::remove_const_t<std::remove_reference_t<decltype(tuple)>>;
      using type=std::tuple<std::tuple_element_t<Index, sourceTupleUnref>...>;
      return type(std::forward< std::tuple_element_t<Index, sourceTupleUnref>>(std::get<Index>(tuple))...);
    }

    template <unsigned I=1> struct UnusedColumns { };

    template <typename T, typename A, typename ...Ts> struct RemoveUnusedIndexes { };
    template <int ...Is, int ...As, typename ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<Is...>,Helper::int_sequence<As...>,Ts...>
    {
      static constexpr auto indexes() { return Helper::int_sequence<As...>(); }
    };
    template <int I, int ...Is, int ...As, class T, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, T, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As..., I>, Ts...>::indexes(); }
    };
    template <int I, int ...Is, int ...As, unsigned J, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, Helper::UnusedColumns<J>, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As...>, Ts...>::indexes(); }
    };
    template <int I, int ...Is, int ...As, unsigned J, class ...Ts> struct RemoveUnusedIndexes<Helper::int_sequence<I, Is...>, Helper::int_sequence<As...>, Helper::UnusedColumns<J> &, Ts...>
    {
      static constexpr auto indexes() { return RemoveUnusedIndexes<Helper::int_sequence<Is...>, Helper::int_sequence<As...>, Ts...>::indexes(); }
    };
    template <typename T> struct StructRemoveUnusedItem
    {
      static T &&remove(T &&value) { return value; }
    };
    template <typename ...Ts> struct StructRemoveUnusedItem<std::tuple<Ts...> &>
    {
      template <int ...Is> static auto removeHelper(Helper::int_sequence<Is...>, std::tuple<Ts...> &data)
      {
        return std::tuple<decltype(StructRemoveUnusedItem<decltype(std::get<Is>(data))>::remove(std::get<Is>(data)))...>(StructRemoveUnusedItem<decltype(std::get<Is>(data))>::remove(std::get<Is>(data))...);
      }
      static auto remove(std::tuple<Ts...> &value)
      {
        auto i=RemoveUnusedIndexes<decltype(Helper::make_int_sequence<sizeof...(Ts)>()), Helper::int_sequence<>, Ts... >::indexes();
        return removeHelper(i, value);
      }
    };

    template <class ...Ts> auto removeUnused(std::tuple<Ts...> &data)
    {
      return StructRemoveUnusedItem<decltype(data) &&>::remove(data);
    }

    template <typename R, typename ...T> auto typeUnusedRefFunctionHelper2(const std::tuple<T...> &)
    {
      return std::function<R(T...)>();
    }

    template <class T> struct RemoveConstRefTupleT
    {
      typedef std::remove_const_t<std::remove_reference_t<T>> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< std::tuple<Ts...> >
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< const std::tuple<Ts...> &>
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };
    template <class ...Ts> struct RemoveConstRefTupleT< std::tuple<Ts...> &>
    {
      typedef std::tuple<typename RemoveConstRefTupleT<Ts>::type...> type;
    };

    template <typename T> using RemoveConstRefTuple = typename RemoveConstRefTupleT<T>::type;

    template <typename R,typename ...T> auto typeUnusedRefFunctionHelper()
    {
      RemoveConstRefTuple<std::tuple<T...>> data;
      auto refPurgedData(Helper::removeUnused(data));
      return typeUnusedRefFunctionHelper2<R>(refPurgedData);
    }
    template <typename R, typename ...T> using TypeUnusedRefFunction=decltype(typeUnusedRefFunctionHelper<R, T...>());
    template <typename ...T> using TypeUnusedRefBoolFunction=decltype(typeUnusedRefFunctionHelper<bool, T...>());

  }
  /// \endcond INTERNAL
}
