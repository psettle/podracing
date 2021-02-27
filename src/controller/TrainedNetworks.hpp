#ifndef TRAINED_NETWORKS_HPP
#define TRAINED_NETWORKS_HPP

#include <string>

// input:
//  0 - max(next_checkpoint_distance / 16000, 1.0)
//  1 - cap(next_checkpoint_angle / 180, -1.0, 1.0) (-0.5 directly left, 0 straight ahead, 0.5
//  directly right) 2 - max(next_next_checkpoint_distance / 16000, 1.0) 3 -
//  cap(next_next_checkpoint_angle / 180, -1.0, 1.0) (-0.5 directly left, 0 straight ahead, 0.5
//  directly right) 4 - max(velocity_magnitude / 1300, 1.0) 5 - cap(velocity_direction / 180,
//  -1.0, 1.0) (-0.5 directly left, 0 straight ahead, 0.5 directly right)

// normalize
//  a1 = (2 * a / (1 + abs(a)));

// output:
// 0 - thrust = <int>(val * 120)
// 1 - clockwise_turn (deg) =  cap(val * 20, -40.0, 40.0) / 2;
// 2 - should_boost = (val >= 0.0)
// 3 - should_shield = (val >= 0.0)
extern std::string const advanced_runner;
extern std::string const advanced_runner_v2;

// input:
//  0 - max(next_checkpoint_distance / 16000, 1.0)
//  1 - cap(next_checkpoint_angle / 180, -1.0, 1.0) (-0.5 directly left, 0 straight ahead, 0.5
//  directly right) 2 - max(velocity_magnitude / 1300, 1.0) 3 - cap(velocity_direction / 180,
//  -1.0, 1.0) (-0.5 directly left, 0 straight ahead, 0.5 directly right)

// normalize
//  a1 = (2 * a / (1 + abs(a)));

// output:
// 0 - thrust = <int>(val * 100)
// 1 - clockwise turn angle (deg) = cap(val, -1.0, 1.0) * 20;
// 2 - should_boost = (val >= 0.5)
// 3 - should_shield = (val >= 0.5)
extern std::string const simple_runner;

#endif