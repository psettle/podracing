#include "GameServer.hpp"
#include <math.h>
#include <cstdlib>
#include <iostream>

GameController::GameController(IPlayer& player0, IPlayer& player1)
    : player_0_(player0), player_1_(player1) {}

void GameController::InitMap() {
  /* Populate checkpoints */
  int num_checkpoints = 2 + std::rand() % 7;

  std::ostringstream map_string;
  map_string << 3 << std::endl;  // laps
  map_string << num_checkpoints << std::endl;

  for (int i = 0; i < num_checkpoints; ++i) {
    map_.push_back(Vec2(std::rand() % 16000, std::rand() % 9000));

    map_string << map_.back().x() << " " << map_.back().y() << std::endl;
    std::cout << map_.back().x() << " " << map_.back().y() << std::endl;
  }

  /* Send map to players */
  player_0_.Setup(map_string.str());
  player_1_.Setup(map_string.str());
}

void GameController::InitPods() {
  static double constexpr seperation = 1000.0;

  /* Pods are placed on a line passing through the checkpoint perpendicular to
   * the vector pointing to the first checkpoint. */
  Vec2 initial_direction = map_[1] - map_[0];
  Vec2 placement_line = Vec2::Perpendicular(initial_direction);
  placement_line.Normalize();

  Vec2 start(map_[0]);

  /* Player 0 gets inside lane */
  player_0_.pod(0).SetPosition(start, placement_line, seperation / 2);
  player_0_.pod(1).SetPosition(start, placement_line, -seperation / 2);

  /* Player 1 gets outside lane */
  player_1_.pod(0).SetPosition(start, placement_line, 3 * seperation / 2);
  player_1_.pod(1).SetPosition(start, placement_line, -3 * seperation / 2);

  /* Pods aim at the first checkpoint */
  player_0_.pod(0).PointAt(map_[1]);
  player_0_.pod(1).PointAt(map_[1]);
  player_1_.pod(0).PointAt(map_[1]);
  player_1_.pod(1).PointAt(map_[1]);
}

int GameController::Turn() {
  std::cout << "frame: " << frame_count << std::endl;
  /* Tell players the current game state. */
  std::ostringstream player_0_data;
  std::ostringstream player_1_data;

  player_0_.GetGameInput(player_0_data);
  player_1_.GetGameInput(player_1_data);

  std::string player_0_input = player_0_data.str() + player_1_data.str();
  std::string player_1_input = player_1_data.str() + player_0_data.str();

  /* Update direction and speed on each pod */
  player_0_.SetInitialTurnConditions(player_0_input);
  player_1_.SetInitialTurnConditions(player_1_input);

  double turn_time_remaining = 1.0;
  while (true) {
    double dt_checkpoint;
    Pod* pod_checkpoint = nullptr;
    double dt_pod;
    Pod* pod_collision_1 = nullptr;
    Pod* pod_collision_2 = nullptr;

    bool checkpoint_collision = GetNextCheckpointCollision(dt_checkpoint, pod_checkpoint);
    bool pod_collision = GetNextPlayerCollision(dt_pod, pod_collision_1, pod_collision_2);

    if (checkpoint_collision && pod_collision) {
      if (dt_checkpoint < dt_pod) {
        pod_collision = false;
      } else {
        checkpoint_collision = false;
      }
    }

    if (checkpoint_collision && dt_checkpoint <= turn_time_remaining) {
      player_0_.AdvancePods(dt_checkpoint);
      player_1_.AdvancePods(dt_checkpoint);
      turn_time_remaining -= dt_checkpoint;
      pod_checkpoint->MakeProgress(dt_checkpoint, map_.size());
      continue;
    }

    if (pod_collision && dt_pod <= turn_time_remaining) {
      player_0_.AdvancePods(dt_pod);
      player_1_.AdvancePods(dt_pod);
      turn_time_remaining -= dt_pod;
      Pod::CollidePods(*pod_collision_1, *pod_collision_2);
      continue;
    }

    break;
  }

  player_0_.AdvancePods(turn_time_remaining);
  player_1_.AdvancePods(turn_time_remaining);

  player_0_.EndTurn();
  player_1_.EndTurn();
  frame_count++;

  if (player_1_.has_lost()) {
    return 0;
  }
  if (player_0_.has_lost()) {
    return 1;
  }
  if (player_0_.has_won() &&
      (!player_1_.has_won() || player_0_.win_time() < player_1_.win_time())) {
    return 0;
  }
  if (player_1_.has_won() &&
      (!player_0_.has_won() || player_1_.win_time() < player_0_.win_time())) {
    return 1;
  }

  return -1;
}

// Return winning player (0 or 1)
int GameController::RunGame() {
  InitMap();
  InitPods();

  while (true) {
    switch (Turn()) {
      case -1:
        /* Nobody won or lost. */
        break;
      case 0:
        /* Player 0 won. */
        return 0;
      case 1:
        /* Player 1 won. */
        return 1;
    }
  }
}

bool GameController::GetNextCheckpointCollision(double& dt, Pod*& pod) {
  /* Set 2.0 as the collision tme for each pod, since we only accept 1 or less as valid. */
  Pod* pods[] = {&player_0_.pod(0), &player_0_.pod(1), &player_1_.pod(0), &player_1_.pod(1)};
  double collision_times[] = {2.0, 2.0, 2.0, 2.0};

  /* Check when each pod will collide with its checkpoint */
  bool found = false;
  for (unsigned int i = 0; i < 4; ++i) {
    double collision_time;
    Pod const* p = pods[i];
    Vec2 const& checkpoint = map_.at(p->next_checkpoint());
    if (GetNextCollision(p->position(), p->velocity(), 0, checkpoint, Vec2(0, 0), 600,
                         collision_time)) {
      if (collision_time < collision_times[i] && collision_time < 1.0) {
        collision_times[i] = collision_time;
        found = true;
      }
    }
  }

  if (!found) {
    return false;
  }

  /* Find the earliest pod collision. */
  dt = 2.0;
  for (unsigned int i = 0; i < 4; ++i) {
    if (collision_times[i] < dt) {
      dt = collision_times[i];
      pod = pods[i];
    }
  }
  return true;
}

bool GameController::GetNextPlayerCollision(double& dt, Pod*& pod1, Pod*& pod2) {
  /* We need to check each pair of pods to see if they will collide, and find the earliest
   * collision. */
  Pod* pods[] = {&player_0_.pod(0), &player_0_.pod(1), &player_1_.pod(0), &player_1_.pod(1)};
  double collision_times[4][4] = {
      {2.0, 2.0, 2.0, 2.0}, {2.0, 2.0, 2.0, 2.0}, {2.0, 2.0, 2.0, 2.0}, {2.0, 2.0, 2.0, 2.0}};

  bool found = false;
  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = i + 1; j < 4; ++j) {
      double collision_time;
      Pod const* p1 = pods[i];
      Pod const* p2 = pods[j];

      if (GetNextCollision(p1->position(), p1->velocity(), 400, p2->position(), p2->velocity(), 400,
                           collision_time)) {
        if (collision_time < collision_times[i][j] && collision_time < 1.0) {
          collision_times[i][j] = collision_time;
          found = true;
        }
      }
    }
  }

  if (!found) {
    return false;
  }

  /* Find the earliest pod collision. */
  dt = 2.0;
  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = i + 1; j < 4; ++j) {
      if (collision_times[i][j] < dt) {
        dt = collision_times[i][j];
        pod1 = pods[i];
        pod2 = pods[j];
      }
    }
  }
  return true;
}

bool GameController::GetNextCollision(Vec2 const& p1, Vec2 const& v1, double r1, Vec2 const& p2,
                                      Vec2 const& v2, double r2, double& dt) {
  Vec2 dp = p2 - p1;
  Vec2 dv = v2 - v1;
  double r = r1 + r2;

  double a = Vec2::Dot(dv, dv);
  double b = 2 * Vec2::Dot(dv, dp);
  double c = Vec2::Dot(dp, dp) - r * r;

  double disc = b * b - 4 * a * c;

  if (disc < 0) {
    /* No collisions */
    return false;
  }

  if (a == 0) {
    /* Travelling in the same direction or both not moving, check if they are already colliding
     */
    if (c < 0) {
      dt = 0.0;
      return true;
    } else {
      return false;
    }
  }

  double t1 = (-b + sqrt(disc)) / (2 * a);
  double t2 = (-b - sqrt(disc)) / (2 * a);

  /* Since the game logic will resolve collisions as they happen, we only care about the
   * smallest time, because that one represets when a collision starts. The larger number
   * represents when the collision ends. */
  double t = std::min(t1, t2);

  if (t < 0) {
    /* Collision occured in negative time, so it never happened or will happen. */
    return false;
  } else {
    dt = t;
    return true;
  }
}