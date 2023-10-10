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

#ifndef NAV2_BEHAVIORS__PLUGINS__STRAFE_HPP_
#define NAV2_BEHAVIORS__PLUGINS__STRAFE_HPP_

#include <memory>

#include "drive_on_heading.hpp"
#include "nav2_msgs/action/strafe.hpp"

using StrafeAction= nav2_msgs::action::Strafe;

namespace nav2_behaviors
{
class Strafe: public DriveOnHeading<nav2_msgs::action::Strafe>
{
public:
  Status onRun(const std::shared_ptr<const StrafeAction::Goal> command) override;
  Status onCycleUpdate() override;

protected:
  /**
   * @brief Check if pose is collision free
   * @param distance Distance to check forward
   * @param cmd_vel current commanded velocity
   * @param pose2d Current pose
   * @return is collision free or not
   */
  bool isCollisionFree(
    const double & distance,
    geometry_msgs::msg::Twist * cmd_vel,
    geometry_msgs::msg::Pose2D & pose2d);

};
}

#endif  // NAV2_BEHAVIORS__PLUGINS__STRAFE_HPP_
