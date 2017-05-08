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
#include <boost/mpl/vector_c.hpp>
#include "storage-facility.hpp"
#include <stencil-composition/stencil-composition.hpp>
#include "helper.hpp"

#ifdef __CUDACC__
#define BACKEND_ARCH gridtools::enumtype::Cuda
#define BACKEND backend<BACKEND_ARCH, GRIDBACKEND, gridtools::enumtype::Block>
#else
#define BACKEND_ARCH gridtools::enumtype::Host
#ifdef BACKEND_BLOCK
#define BACKEND backend<BACKEND_ARCH, GRIDBACKEND, gridtools::enumtype::Block>
#else
#define BACKEND backend<BACKEND_ARCH, GRIDBACKEND, gridtools::enumtype::Naive>
#endif
#endif

typedef gridtools::storage_traits<BACKEND_ARCH>::storage_info_t<0, 3>
    storage_info_3d_t;
typedef gridtools::storage_traits<BACKEND_ARCH>::data_store_t<
    gridtools::float_type, storage_info_3d_t> data_store_3d_t;

typedef gridtools::storage_traits<BACKEND_ARCH>::storage_info_t<0, 2>
    storage_info_2d_t;
typedef gridtools::storage_traits<BACKEND_ARCH>::data_store_t<
    gridtools::float_type, storage_info_2d_t> data_store_2d_t;

enum class dycore_param { u, v, w, tp, fc, utens, vtens, wtens, hdmask };
using dycore_repo_info_t = boost::mpl::map<
    boost::mpl::pair<
        data_store_3d_t,
        boost::mpl::vector_c<
            dycore_param, (long int)dycore_param::u, (long int)dycore_param::v,
            (long int)dycore_param::w, (long int)dycore_param::tp,
            (long int)dycore_param::utens, (long int)dycore_param::vtens,
            (long int)dycore_param::wtens, (long int)dycore_param::hdmask>>,
    boost::mpl::pair<
        data_store_2d_t,
        boost::mpl::vector_c<dycore_param, (long int)dycore_param::fc>>>;

enum class fast_waves_sc_param { lgsA, lgsB, lgsC, lgsRHS, rCosPhi };
using fw_sc_repo_info_t = boost::mpl::map<
    boost::mpl::pair<
        data_store_3d_t,
        boost::mpl::vector_c<fast_waves_sc_param,
                             (long int)fast_waves_sc_param::lgsA,
                             (long int)fast_waves_sc_param::lgsB,
                             (long int)fast_waves_sc_param::lgsC,
                             (long int)fast_waves_sc_param::lgsRHS,
                             (long int)fast_waves_sc_param::rCosPhi>>,
    boost::mpl::pair<
        data_store_2d_t,
        boost::mpl::vector_c<fast_waves_sc_param,
                             (long int)fast_waves_sc_param::rCosPhi>>>;

enum class vadvect { data, datatens, fc };
using list_vadvect_params = boost::mpl::map<
    boost::mpl::pair<data_store_3d_t,
                     boost::mpl::vector_c<vadvect, (long int)vadvect::data,
                                          (long int)vadvect::datatens>>,
    boost::mpl::pair<data_store_2d_t,
                     boost::mpl::vector_c<vadvect, (long int)vadvect::fc>>>;
