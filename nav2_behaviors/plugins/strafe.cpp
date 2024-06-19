// Copyright (c) 2022 Joshua Wallace
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "nav2_behaviors/plugins/strafe.hpp"

namespace nav2_behaviors
{

ResultStatus Strafe::onRun(const std::shared_ptr<const StrafeAction::Goal> command)
{
  if (command->target.x != 0.0 || command->target.z != 0.0) {
    RCLCPP_INFO(
      logger_,
      "Strafing in y axis only");
    return ResultStatus{Status::FAILED, StrafeActionResult::INVALID_INPUT};
  }

  // Silently ensure that both the speed and direction are negative.
  command_x_ = command->target.y;
  command_speed_ = command->speed;
  command_time_allowance_ = command->time_allowance;

  end_time_ = this->clock_->now() + command_time_allowance_;

  if (!nav2_util::getCurrentPose(
      initial_pose_, *tf_, global_frame_, robot_base_frame_,
      transform_tolerance_))
  {
    RCLCPP_ERROR(logger_, "Initial robot pose is not available.");
    return ResultStatus{Status::FAILED, StrafeActionResult::TF_ERROR};
  }

  return ResultStatus{Status::SUCCEEDED, StrafeActionResult::NONE};
}

ResultStatus Strafe::onCycleUpdate()
{
    rclcpp::Duration time_remaining = end_time_ - this->clock_->now();
    if (time_remaining.seconds() < 0.0 && command_time_allowance_.seconds() > 0.0) {
      this->stopRobot();
      RCLCPP_WARN(
        this->logger_,
        "Exceeded time allowance before reaching the DriveOnHeading goal - Exiting DriveOnHeading");
      return ResultStatus{Status::FAILED, StrafeActionResult::NONE};
    }

    geometry_msgs::msg::PoseStamped current_pose;
    if (!nav2_util::getCurrentPose(
        current_pose, *this->tf_, this->global_frame_, this->robot_base_frame_,
        this->transform_tolerance_))
    {
      RCLCPP_ERROR(this->logger_, "Current robot pose is not available.");
      return ResultStatus{Status::FAILED, StrafeActionResult::TF_ERROR};
    }

    double diff_x = initial_pose_.pose.position.x - current_pose.pose.position.x;
    double diff_y = initial_pose_.pose.position.y - current_pose.pose.position.y;
    double distance = hypot(diff_x, diff_y);

    feedback_->distance_traveled = distance;
    this->action_server_->publish_feedback(feedback_);

    if (distance >= std::fabs(command_x_)) {
      this->stopRobot();
      return ResultStatus{Status::SUCCEEDED, StrafeActionResult::NONE};
    }

    auto cmd_vel = std::make_unique<geometry_msgs::msg::TwistStamped>();
    cmd_vel->header.stamp = clock_->now();
    cmd_vel->twist.linear.x = 0.0;
    cmd_vel->twist.angular.z = 0.0;
    cmd_vel->twist.linear.y = command_speed_;

    geometry_msgs::msg::Pose2D pose2d;
    pose2d.x = current_pose.pose.position.x;
    pose2d.y = current_pose.pose.position.y;
    pose2d.theta = tf2::getYaw(current_pose.pose.orientation);

    if (!isCollisionFree(distance, cmd_vel.get(), pose2d)) {
      this->stopRobot();
      RCLCPP_WARN(this->logger_, "Collision Ahead - Exiting Strafing");
      return ResultStatus{Status::FAILED, StrafeActionResult::COLLISION_AHEAD};
    }

    this->vel_pub_->publish(std::move(cmd_vel));

    return ResultStatus{Status::RUNNING, StrafeActionResult::NONE};
  }

bool Strafe::isCollisionFree(
    const double & distance,
    geometry_msgs::msg::TwistStamped * cmd_vel,
    geometry_msgs::msg::Pose2D & pose2d)
  {
    // Simulate ahead by simulate_ahead_time_ in this->cycle_frequency_ increments
    int cycle_count = 0;
    double sim_position_change;
    const double diff_dist = abs(command_x_) - distance;
    const int max_cycle_count = static_cast<int>(this->cycle_frequency_ * simulate_ahead_time_);
    geometry_msgs::msg::Pose2D init_pose = pose2d;
    bool fetch_data = true;

    while (cycle_count < max_cycle_count) {
      sim_position_change = cmd_vel->twist.linear.y * (cycle_count / this->cycle_frequency_);
      pose2d.x = init_pose.x + sim_position_change * cos(init_pose.theta);
      pose2d.y = init_pose.y + sim_position_change * sin(init_pose.theta);
      cycle_count++;

      if (diff_dist - abs(sim_position_change) <= 0.) {
        break;
      }

      if (!this->local_collision_checker_->isCollisionFree(pose2d, fetch_data)) {
        return false;
      }
      fetch_data = false;
    }
    return true;
  }
}  // namespace nav2_behaviors

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(nav2_behaviors::Strafe, nav2_core::Behavior)
