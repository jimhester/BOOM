#ifndef BOOM_STATE_SPACE_STUDENT_MVSS_POSTERIOR_SAMPLER_HPP_
#define BOOM_STATE_SPACE_STUDENT_MVSS_POSTERIOR_SAMPLER_HPP_
/*
  Copyright (C) 2005-2022 Steven L. Scott

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with this library; if not, write to the Free Software Foundation, Inc., 51
  Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "Models/PosteriorSamplers/PosteriorSampler.hpp"
#include "Models/Glm/PosteriorSamplers/TDataImputer.hpp"
#include "Models/StateSpace/Multivariate/StudentMvssRegressionModel.hpp"
#include "Models/StateSpace/Multivariate/PosteriorSamplers/MultivariateStateSpaceModelSampler.hpp"

namespace BOOM {

  class StudentMvssPosteriorSampler
      : public MultivariateStateSpaceModelSampler {
   public:
    StudentMvssPosteriorSampler(
        StudentMvssRegressionModel *model,
        RNG &seeding_rng = GlobalRng::rng);

    StudentMvssPosteriorSampler * clone_to_new_host(
        StudentMvssRegressionModel *new_host) const;

    void impute_nonstate_latent_data() override {
      model_->impute_student_weights(rng());
    }

    void clear_complete_data_sufficient_statistics();

   private:
    StudentMvssRegressionModel *model_;
    TDataImputer data_imputer_;
  };


}  // namespace BOOM

#endif  //  BOOM_STATE_SPACE_STUDENT_MVSS_POSTERIOR_SAMPLER_HPP_
