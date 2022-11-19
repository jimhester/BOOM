#ifndef BOOM_MULTIVARIATE_STATE_SPACE_DATA_POLICY_HPP_
#define BOOM_MULTIVARIATE_STATE_SPACE_DATA_POLICY_HPP_
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

#include "cpputil/Ptr.hpp"
#include <functional>
#include <vector>

namespace BOOM {

  // Multivariate state space regression models hold regression data of various
  // types.  Each data point describes one value of one time series at a single
  // time point.
  //
  // This data policy exists to help organize the time-aspect
  template <class DATA_TYPE>
  class MultivariateStateSpaceRegressionDataPolicy {
   public:
    MultivariateStateSpaceRegressionDataPolicy(int nseries)
        : nseries_(nseries),
          time_dimension_(0),
          missing_()
    {}

    int nseries() const {return nseries_;}
    int time_dimension() const {return time_dimension_;}

    void clear_data() {
      time_dimension_ = 0;
      observed_.clear();
      data_indices_.clear();
      raw_data_.clear();
      call_observers();
    }

    // Add a data point to the model, adjusting
    void add_data(const Ptr<DATA_TYPE> &data_point) {
      time_dimension_ = std::max<int>(time_dimension_,
                                      1 + data_point->timestamp());
      data_indices_[data_point->series()][data_point->timestamp()] =
          raw_data_.size();
      raw_data_.push_back(data_point);
      call_observers();
      while (observed_.size() <= data_point->timestamp()) {
        Selector all_missing(nseries_, false);
        observed_.push_back(all_missing);
      }
      observed_[data_point->timestamp()].add(data_point->series());
    }

    void call_observers() {
      for (auto &obs : data_change_observers_) {
        obs();
      }
    }

    const Ptr<DATA_TYPE> & data_point(int64_t index) const {
      return raw_data_[index];
    }

    Ptr<DATA_TYPE> & data_point(int64_t index) {
      return raw_data_[index];
    }

    Ptr<DATA_TYPE> & data_point(int series, int time) {
      int64_t index = data_index(series, time);
      if (index < 0) {
        return missing_;
      } else {
        return raw_data_[index];
      }
    }

    const Ptr<DATA_TYPE> & data_point(int series, int time) const {
      int64_t index = data_index(series, time);
      if (index < 0) {
        return missing_;
      } else {
        return raw_data_[index];
      }
    }

    // The index in raw_data_ corresponding to the data point for a specific
    // series and time point.  If that data point does not exist, then -1 is
    // returned.
    int64_t data_index(int series, int time) const {
      const auto &series_it = data_indices_.find(series);
      if (series_it == data_indices_.end()) {
        return -1;
      }
      const auto & time_it(series_it->second.find(time));
      if (time_it == series_it->second.end()) {
        return -1;
      }
      return time_it->second;
    }

    void add_observer(std::function<void(void)> observer) {
      data_change_observers_.push_back(observer);
    }

    const Selector &observed(int time) const {
      return observed_[time];
    }

    void set_observed_status(int t, const Selector &observed) {
      if (observed.nvars_possible() != observed_[0].nvars_possible()) {
        report_error("Wrong size Selector passed to set_observed_status.");
      }
      observed_[t] = observed;
    }

    size_t total_sample_size() const {
      return raw_data_.size();
    }

   private:
    int nseries_;
    int time_dimension_;

    // data_indices_[series][time] gives the index of the corresponding element
    // of raw_data_;
    std::map<int, std::map<int, int64_t>> data_indices_;

    std::vector<Ptr<DATA_TYPE>> raw_data_;
    std::vector<Selector> observed_;

    Ptr<DATA_TYPE> missing_;
    // Every funtion in this vector will be called whenever data is added or
    // cleared.
    std::vector<std::function<void(void)>> data_change_observers_;
  };


}  // namespace BOOM

#endif  // BOOM_MULTIVARIATE_STATE_SPACE_DATA_POLICY_HPP_
