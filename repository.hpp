/*
  GridTools Libraries

  Copyright (c) 2017, ETH Zurich and MeteoSwiss
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  For information: http://eth-cscs.github.io/gridtools/
*/

#include <stencil-composition/stencil-composition.hpp>
#include "storage-facility.hpp"
#include "param_definitions.hpp"

template <typename T, typename Elem> struct concat;

template <template <class...> class Cont, typename Elem, typename... Vals>
struct concat<Cont<Vals...>, Elem> {
  using type = Cont<Vals..., Elem>;
};

template <int N, template <class...> class Cont, typename Elem, typename R>
struct repeat {
  using type =
      typename repeat<N - 1, Cont, Elem, typename concat<R, Elem>::type>::type;
};

template <template <class...> class Cont, typename Elem, typename... V>
struct repeat<0, Cont, Elem, Cont<V...>> {
  using type = Cont<V...>;
};

template <typename Tuple, int N, typename DataStore> struct create_tuple {
  template <typename Arg1, typename... Arg>
  static constexpr Tuple apply(Arg1 arg1, Arg... args) {
    return create_tuple<Tuple, N - 1, DataStore>::apply(arg1, args..., arg1);
  }
};

template <typename Tuple, typename DataStore>
struct create_tuple<Tuple, 1, DataStore> {
  template <typename Arg1, typename... Arg>
  static constexpr Tuple apply(Arg1 arg1, Arg... args) {
    return std::make_tuple(DataStore(arg1), DataStore(args)...);
  }
};

template <typename Tuple, typename DataStore>
struct create_tuple<Tuple, 0, DataStore> {
  template <typename... Arg> static constexpr std::tuple<> apply(Arg... args) {
    return std::make_tuple();
  }
};

template <typename EnumT, typename repo_info> struct repository {

  using fields_3d_t = typename boost::mpl::at<repo_info, data_store_3d_t>::type;
  using fields_2d_t = typename boost::mpl::at<repo_info, data_store_2d_t>::type;

  static constexpr unsigned int fields_3d_size =
      boost::mpl::size<fields_3d_t>::value;
  static constexpr unsigned int fields_2d_size =
      boost::mpl::size<fields_2d_t>::value;

  using field_3d_tuple_t = typename repeat<fields_3d_size, std::tuple,
                                           data_store_3d_t, std::tuple<>>::type;

  using field_2d_tuple_t = typename repeat<fields_2d_size, std::tuple,
                                           data_store_2d_t, std::tuple<>>::type;

  template <typename Tuple> struct allocate_data_stores {
    Tuple &m_tuple;
    allocate_data_stores(Tuple &tuple) : m_tuple(tuple) {}
    template <typename Index> void operator()(Index const &) {
      std::get<Index::value>(m_tuple).allocate();
    }
  };

  repository()
      : m_sinfo_3d(10, 10, 10), m_sinfo_2d(10, 10),
        m_fields_3d(create_tuple<field_3d_tuple_t, fields_3d_size,
                                 data_store_3d_t>::apply(m_sinfo_3d)),
        m_fields_2d(create_tuple<field_2d_tuple_t, fields_2d_size,
                                 data_store_2d_t>::apply(m_sinfo_2d)) {
    boost::mpl::for_each<
        boost::mpl::range_c<int, 0, std::tuple_size<field_3d_tuple_t>::value>>(
        allocate_data_stores<field_3d_tuple_t>(m_fields_3d));
    boost::mpl::for_each<
        boost::mpl::range_c<int, 0, std::tuple_size<field_2d_tuple_t>::value>>(
        allocate_data_stores<field_2d_tuple_t>(m_fields_2d));
  }

  template <EnumT param> struct param_storage {
    using type = typename std::conditional<
        !boost::is_same<
            typename boost::mpl::find<
                fields_3d_t, boost::mpl::integral_c<EnumT, param>>::type,
            typename boost::mpl::end<fields_3d_t>::type>::value,
        data_store_3d_t, data_store_2d_t>::type;
  };

  template <EnumT param> struct is_3d_storage {
    using type = typename boost::mpl::not_<typename boost::is_same<
        typename boost::mpl::find<fields_3d_t,
                                  boost::mpl::integral_c<EnumT, param>>::type,
        typename boost::mpl::end<fields_3d_t>::type>::type>::type;
  };

  template <EnumT param> struct is_2d_storage {
    using type = typename boost::mpl::not_<typename boost::is_same<
        typename boost::mpl::find<fields_2d_t,
                                  boost::mpl::integral_c<EnumT, param>>::type,
        typename boost::mpl::end<fields_2d_t>::type>::type>::type;
  };

  template <EnumT param>
  typename param_storage<param>::type get_st(
      typename std::enable_if<is_3d_storage<param>::type::value>::type * = 0) {
    constexpr unsigned int pos = boost::mpl::find<
        fields_3d_t, boost::mpl::integral_c<EnumT, param>>::type::pos::value;
    return std::get<pos>(m_fields_3d);
  }

  template <EnumT param>
  typename param_storage<param>::type get_st(
      typename std::enable_if<is_2d_storage<param>::type::value>::type * = 0) {
    constexpr unsigned int pos = boost::mpl::find<
        fields_2d_t, boost::mpl::integral_c<EnumT, param>>::type::pos::value;
    return std::get<pos>(m_fields_2d);
  }

private:
  storage_info_3d_t m_sinfo_3d;
  storage_info_2d_t m_sinfo_2d;

  field_3d_tuple_t m_fields_3d;
  field_2d_tuple_t m_fields_2d;
};
