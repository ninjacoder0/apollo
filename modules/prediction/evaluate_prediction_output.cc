/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @brief Generate trajectory-point-level evaluation results
 * @param input_feature_proto_file: Feature proto with true future status
 */
#include "cyber/common/file.h"

#include "modules/common/adapters/proto/adapter_config.pb.h"
#include "modules/prediction/common/prediction_system_gflags.h"
#include "modules/prediction/common/prediction_gflags.h"
#include "modules/prediction/proto/feature.pb.h"
#include "modules/prediction/proto/offline_features.pb.h"
#include "modules/prediction/proto/prediction_conf.pb.h"
#include "modules/prediction/proto/prediction_output_evaluation.pb.h"
#include "modules/prediction/container/container_manager.h"
#include "modules/prediction/container/obstacles/obstacles_container.h"
#include "modules/prediction/evaluator/evaluator_manager.h"
#include "modules/prediction/predictor/predictor_manager.h"

namespace apollo {
namespace prediction {

void Initialize() {
  PredictionConf prediction_conf;
  if (!apollo::common::util::GetProtoFromFile(
           FLAGS_prediction_conf_file, &prediction_conf)) {
    AERROR << "Unable to load prediction conf file: "
           << FLAGS_prediction_conf_file;
    return;
  }
  ADEBUG << "Prediction config file is loaded into: "
            << prediction_conf.ShortDebugString();

  apollo::common::adapter::AdapterManagerConfig adapter_conf;
  if (!apollo::common::util::GetProtoFromFile(
    FLAGS_prediction_adapter_config_filename, &adapter_conf)) {
    AERROR << "Unable to load adapter conf file: "
           << FLAGS_prediction_adapter_config_filename;
    return;
  }
  // Initialization of all managers
  ContainerManager::Instance()->Init(adapter_conf);
  EvaluatorManager::Instance()->Init(prediction_conf);
  PredictorManager::Instance()->Init(prediction_conf);
}

double CorrectlyPredictedPortionOnLane(
    const Obstacle* obstacle_ptr, const double time_range,
    int* num_predicted_trajectory) {
  // TODO(kechxu) implement
  return 0.0;
}

double CorrectlyPredictedPortionJunction(
    const Obstacle* obstacle_ptr, const double time_range,
    int* num_predicted_trajectory) {
  // TODO(kechxu) implement
  return 0.0;
}

void UpdateMetrics(const int num_trajectory, const double correct_portion,
    TrajectoryEvaluationMetrics* const metrics) {
  metrics->set_num_frame_obstacle(metrics->num_frame_obstacle() + 1);
  metrics->set_num_predicted_trajectory(
      metrics->num_predicted_trajectory() + num_trajectory);
  metrics->set_num_correctly_predicted_frame_obstacle(
      metrics->num_correctly_predicted_frame_obstacle() + correct_portion);
}

void ComputeResults(TrajectoryEvaluationMetrics* const metrics) {
  if (metrics->num_frame_obstacle() == 0 ||
      metrics->num_predicted_trajectory() == 0) {
    return;
  }
  metrics->set_recall(metrics->num_correctly_predicted_frame_obstacle() /
                     metrics->num_frame_obstacle());
  metrics->set_recall(metrics->num_correctly_predicted_frame_obstacle() /
                     metrics->num_predicted_trajectory());
  // TODO(kechxu) compute sum_squared_error
}

TrajectoryEvaluationMetricsGroup Evaluate(
    const Features& features_with_future_status) {
  TrajectoryEvaluationMetrics on_lane_metrics;
  TrajectoryEvaluationMetrics junction_metrics;
  TrajectoryEvaluationMetricsGroup metrics_group;
  auto obstacles_container_ptr =
      ContainerManager::Instance()->GetContainer<ObstaclesContainer>(
        apollo::common::adapter::AdapterConfig::PERCEPTION_OBSTACLES);
  for (const Feature& feature : features_with_future_status.feature()) {
    obstacles_container_ptr->InsertFeatureProto(feature);
    Obstacle* obstacle_ptr = obstacles_container_ptr->GetObstacle(feature.id());
    if (obstacle_ptr->HasJunctionFeatureWithExits()) {
      int num_trajectory = 0;
      double correct_portion = CorrectlyPredictedPortionJunction(
          obstacle_ptr, 3.0, &num_trajectory);
      UpdateMetrics(num_trajectory, correct_portion, &junction_metrics);
    } else if (obstacle_ptr->IsOnLane()) {
      int num_trajectory = 0;
      double correct_portion = CorrectlyPredictedPortionOnLane(
          obstacle_ptr, 3.0, &num_trajectory);
      UpdateMetrics(num_trajectory, correct_portion, &on_lane_metrics);
    }
  }
  ComputeResults(&junction_metrics);
  ComputeResults(&on_lane_metrics);
  metrics_group.mutable_junction_metrics()->CopyFrom(junction_metrics);
  metrics_group.mutable_on_lane_metrics()->CopyFrom(on_lane_metrics);
  return metrics_group;
}

}  // namespace prediction
}  // namespace apollo

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  apollo::prediction::Initialize();
  // TODO(kechxu) Load feature proto and run Evaluate
}
