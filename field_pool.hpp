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

#pragma once
#include <boost/mpl/set_c.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
#include <boost/type_traits/is_same.hpp>
#include <map>
#include "param_definitions.hpp"
#include "repository.hpp"

#define ARG(data_store) gridtools::arg<__COUNTER__, data_store>

template <typename Variadic, typename T> struct variadic_concat;

template <typename T, typename... Vars>
struct variadic_concat<gridtools::variadic_typedef<Vars...>, T> {
  using type = gridtools::variadic_typedef<Vars..., T>;
};

template <typename Variadic> struct variadic_to_tuple;

template <typename... Vars>
struct variadic_to_tuple<gridtools::variadic_typedef<Vars...>> {
  using type = std::tuple<Vars...>;
};

template <typename Map> struct map_to_tuple {

  template <typename Variadic, typename Pair> struct insert {
    using type =
        typename variadic_concat<Variadic,
                                 typename boost::mpl::second<Pair>::type>::type;
  };

  using vt_t =
      typename boost::mpl::fold<Map, gridtools::variadic_typedef<>,
                                insert<boost::mpl::_1, boost::mpl::_2>>::type;

  using type = typename variadic_to_tuple<vt_t>::type;
};

struct field_pool {

  using tuple_t =
      std::tuple<repository<dycore_param, dycore_repo_info_t>,
                 repository<fast_waves_sc_param, fw_sc_repo_info_t>>;

  using context_list_t = boost::mpl::vector<dycore_param, fast_waves_sc_param>;

  template <typename... Cont> struct build_arg_map {

    template <typename Map, typename Key, typename data_store_t>
    struct insert_key {
      using type = typename boost::mpl::insert<
          Map, boost::mpl::pair<Key, ARG(data_store_t)>>::type;
    };

    template <typename Map, typename DPPair> struct insert_in_map {
      using enum_seq = typename boost::mpl::second<DPPair>::type;
      using data_store_t = typename boost::mpl::first<DPPair>::type;
      using enum_type = typename enum_seq::value_type;

      using type = typename boost::mpl::fold<
          enum_seq, Map,
          insert_key<boost::mpl::_1, boost::mpl::_2, data_store_t>>::type;
    };

    template <typename Map, typename MapDP> struct insert {
      using type = typename boost::mpl::fold<
          MapDP, Map, insert_in_map<boost::mpl::_1, boost::mpl::_2>>::type;
    };

    using type =
        typename boost::mpl::fold<boost::mpl::vector<Cont...>,
                                  boost::mpl::map0<>,
                                  insert<boost::mpl::_1, boost::mpl::_2>>::type;
  };

  using args_t = typename build_arg_map<dycore_repo_info_t, fw_sc_repo_info_t,
                                        list_vadvect_params>::type;

  using args_tuple_t = typename map_to_tuple<args_t>::type;

  template <typename HMap, typename Pair> struct insert_nindex {
    using index_t = typename boost::mpl::first<HMap>::type;
    using map_t = typename boost::mpl::second<HMap>::type;

    using indexpt_t = boost::mpl::integral_c<int, index_t::value + 1>;
    using type = boost::mpl::pair<
        indexpt_t,
        typename boost::mpl::insert<
            map_t, boost::mpl::pair<typename boost::mpl::first<Pair>::type,
                                    indexpt_t>>::type>;
  };

  using args_to_index_ht = typename boost::mpl::fold<
      args_t,
      boost::mpl::pair<boost::mpl::integral_c<int, -1>, boost::mpl::map0<>>,
      insert_nindex<boost::mpl::_1, boost::mpl::_2>>::type;

  using args_to_index_t = typename boost::mpl::second<args_to_index_ht>::type;

  using args_vector_t = typename boost::mpl::fold<
      args_t, boost::mpl::vector0<>,
      boost::mpl::push_back<boost::mpl::_1, boost::mpl::_2>>::type;

private:
  static field_pool *m_field_pool;

  tuple_t m_repos;
  std::array<unsigned int, boost::mpl::size<context_list_t>::value>
      m_active_context;
  args_tuple_t m_args_tuple;

public:
  static field_pool &get_instance();

  // for value-initialization of the array
  field_pool() : m_active_context{} {}

  template <typename EnumT, EnumT param> struct param_storage {
    using type = typename std::tuple_element<
        boost::mpl::find<context_list_t, EnumT>::type::pos::value,
        tuple_t>::type::template param_storage<param>::type;
  };

  template <typename EnumT, EnumT param>
  typename param_storage<EnumT, param>::type get_st() {
    if (!m_active_context[boost::mpl::find<context_list_t,
                                           EnumT>::type::pos::value])
      throw(std::runtime_error("Can not access storage out of context"));

    return std::get<boost::mpl::find<context_list_t, EnumT>::type::pos::value>(
               m_repos)
        .get_st<param>();
  }
  template <typename EnumT> void activate_context() {
    m_active_context[boost::mpl::find<context_list_t,
                                      EnumT>::type::pos::value] = true;
  }

  template <typename EnumT> void deactivate_context() {
    m_active_context[boost::mpl::find<context_list_t,
                                      EnumT>::type::pos::value] = false;
  }

  template <typename... Params> void bind_all_args() {
    // here we should bind all the prognostic and constant fields placeholders
  }

  template <typename EnumT, EnumT param, typename Storage>
  void bind_arg(Storage st) {
    get_arg<EnumT, param>() = st;
  }

  template <typename EnumT, EnumT param>
  typename boost::mpl::at<args_t, boost::mpl::integral_c<EnumT, param>>::type
  get_arg() {
    GRIDTOOLS_STATIC_ASSERT(
        (boost::mpl::has_key<args_t,
                             boost::mpl::integral_c<EnumT, param>>::value),
        "Error");

    using idx =
        typename boost::mpl::at<args_to_index_t,
                                boost::mpl::integral_c<EnumT, param>>::type;
    return std::get<idx::value>(m_args_tuple);
  }
};
