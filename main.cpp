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
#include "field_pool.hpp"

template <typename... F> std::tuple<F &&...> input(F &&... f) {
  return std::forward_as_tuple(f...);
}

template <typename... F> std::tuple<F &&...> output(F &&... f) {
  return std::forward_as_tuple(f...);
}

struct vertical_advection {

  vertical_advection() {
    field_pool &fpool = field_pool::get_instance();

    // we create the placeholders that later we will bind to actual storages
    // This stencil can be applied to multiple prognostic fields, therefore
    // it defines a specific context, vadvect, that defines joker placeholder
    // that can be bound to different prognostic field storages (u,v,w...)
    auto p_data = fpool.get_arg<vadvect, vadvect::data>();
    auto p_fc = fpool.get_arg<vadvect, vadvect::fc>();
    auto p_data_tens = fpool.get_arg<vadvect, vadvect::datatens>();
    // Not all fields will be passed by args to the stencil workflow.
    // Constant fields are bound at initialization, and not changed
    // through the run of the model. Therefore in the example below,
    // hdmask is not a placeholder specific to the vertical advection context
    // but belongs instead to the dycore as a global prognostic field
    auto p_hdmask = fpool.get_arg<dycore_param, dycore_param::hdmask>();

    //    gridtools::aggregator_type<  > domain(p_data(), p_fc(), p_data_tens(),
    //    p_hdmask());

    //    m_stencil = gridtools::make_computation<gridtools::BACKEND>(
    //        domain, grid,
    //        gridtools::make_multistage // mss_descriptor
    //        (execute<forward>(),
    //         gridtools::make_stage<vadvt_functor>(....)));
  }

  template <typename InputTuple, typename OutputTuple>
  void single_vertical_advection(InputTuple &&it, OutputTuple &&ot) {
    field_pool &fpool = field_pool::get_instance();

    auto in = std::get<0>(it);
    auto fc = std::get<1>(it);
    auto out = std::get<0>(ot);
    // here we bind the placeholders of the vadv stencil to the actual
    // parameters passed
    fpool.bind_arg<vadvect, vadvect::data>(in);
    fpool.bind_arg<vadvect, vadvect::fc>(fc);
    fpool.bind_arg<vadvect, vadvect::datatens>(out);

    // and run the stencil
    //    m_stencil->run();
  }

  template <typename InputTuple, typename OutputTuple>
  void run(InputTuple &&it, OutputTuple &&ot) {
    field_pool &fpool = field_pool::get_instance();

    // we unpack the multiple fields to which we apply this operator
    auto u = std::get<0>(it);
    auto v = std::get<1>(it);
    auto w = std::get<2>(it);
    auto fc = std::get<3>(it);

    auto utens = std::get<0>(ot);
    auto vtens = std::get<1>(ot);
    auto wtens = std::get<2>(ot);

    // and call the vertical advection operator for each prognostic field
    single_vertical_advection(input(u, fc), output(utens));
    single_vertical_advection(input(v, fc), output(vtens));
    single_vertical_advection(input(w, fc), output(wtens));
  }

private:
  std::shared_ptr<gridtools::computation<void>> m_stencil;
};

void fast_waves_sc() {
  field_pool &fpool = field_pool::get_instance();

  // we enter into the fast waves context, from this point on we can request
  // fields associated with the fast waves context
  fpool.activate_context<fast_waves_sc_param>();

  // we do an initial binding of the fast waves placholders to storages.
  fpool.bind_all_args<fw_sc_repo_info_t>();

  auto lgsA = fpool.get_st<fast_waves_sc_param, fast_waves_sc_param::lgsA>();

  fpool.deactivate_context<fast_waves_sc_param>();
}

int main(int argc, char **argv) {

  vertical_advection va;
  field_pool &fpool = field_pool::get_instance();

  fpool.activate_context<dycore_param>();

  // for all the params defined in the enum class of parameters (dycore, fw,
  // etc)
  // we do an initial binding of placholders to storages
  // Not all storages are passedby argument (i.e. constant fields), so
  // this initial bind will make sure that all prognostic fields placeholders of
  // the stencils are properly setup
  fpool.bind_all_args<dycore_repo_info_t>();

  // Then we can request all prognostic fields that we will
  // connect the different stencil operators as input/output dependencies
  auto u = fpool.get_st<dycore_param, dycore_param::u>();
  auto v = fpool.get_st<dycore_param, dycore_param::v>();
  auto w = fpool.get_st<dycore_param, dycore_param::w>();
  auto fc = fpool.get_st<dycore_param, dycore_param::fc>();
  auto utens = fpool.get_st<dycore_param, dycore_param::utens>();
  auto vtens = fpool.get_st<dycore_param, dycore_param::vtens>();
  auto wtens = fpool.get_st<dycore_param, dycore_param::wtens>();

  va.run(input(u, v, w, fc), output(utens, vtens, wtens));

  // The folowing access will throw an exception, since we did not create yet
  // the context of the fast waves sc. Field access in a scope out of the
  // context of
  // can not be accessed.
  //
  //  auto lgsA = fpool.get_st<fast_waves_sc_param,
  //  fast_waves_sc_param::lgsA>();

  fast_waves_sc();

  // we get out of the dycore context. Beyong this line we can not access any
  // dycore prognostic field
  fpool.deactivate_context<dycore_param>();
}
