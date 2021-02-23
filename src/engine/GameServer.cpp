#include "GameServer.hpp"
#include <math.h>
#include <cstdlib>
#include <iostream>

void GameController::AddPlayer(IPlayer& player) {
  players_.push_back(std::make_unique<Player>(player));
}

void GameController::InitMap() {
  /* Populate checkpoints */
  int num_checkpoints = 2 + std::rand() % 7;

  std::ostringstream map_string;
  map_string << 3 << std::endl;  // laps
  map_string << num_checkpoints << std::endl;

  for (int i = 0; i < num_checkpoints; ++i) {
    while (true) {
      double closest = INFINITY;
      Vec2 candidate(std::rand() % 16000, std::rand() % 9000);
      for (unsigned int j = 0; j < map_.size(); ++j) {
        double distance = (map_[j] - candidate).Length();

        if (distance < closest) {
          closest = distance;
        }
      }

      if (closest > 1200) {
        map_.push_back(candidate);
        break;
      }
    }
    map_string << map_.back().x() << " " << map_.back().y() << std::endl;
  }

  /* Send map to players */
  for (auto& player : players_) {
    player->Setup(map_string.str());
  }
}

void GameController::InitPods() {
  static double constexpr kSeperation = 1000.0;

  /* Pods are placed on a line passing through the checkpoint perpendicular to
   * the vector pointing to the first checkpoint. */
  Vec2 initial_direction = map_[1] - map_[0];
  Vec2 placement_line = Vec2::Perpendicular(initial_direction);
  placement_line.Normalize();

  /* Player 0 gets inside lane */
  double seperation = 500;
  for (auto& player : players_) {
    player->InitPods(map_[0], placement_line, seperation, map_[1]);
    seperation += kSeperation;
  }
}

int GameController::Turn() {
  /* Tell players the current game state. */
  std::vector<std::ostringstream> player_data(players_.size());

  for (unsigned int i = 0; i < players_.size(); ++i) {
    players_[i]->GetGameInput(player_data[i]);
  }

  for (unsigned int i = 0; i < players_.size(); ++i) {
    std::string player_input = player_data[i].str();

    for (unsigned int j = 0; j < players_.size(); ++j) {
      if (i == j) {
        continue;
      }

      player_input += player_data[j].str();
    }

    players_[i]->SetInitialTurnConditions(player_input, frame_count == 0);
  }

  double turn_time_remaining = 1.0;
  for (unsigned int t = 0; t < 1000; ++t) {
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
      for (auto& player : players_) {
        player->AdvancePods(dt_checkpoint);
      }
      turn_time_remaining -= dt_checkpoint;
      pod_checkpoint->MakeProgress(dt_checkpoint, map_.size());
      continue;
    }

    if (pod_collision && dt_pod <= turn_time_remaining) {
      for (auto& player : players_) {
        player->AdvancePods(dt_pod);
      }
      turn_time_remaining -= dt_pod;
      Pod::CollidePods(*pod_collision_1, *pod_collision_2);
      continue;
    }

    break;
  }

  for (auto& player : players_) {
    player->AdvancePods(turn_time_remaining);
    player->EndTurn();
  }

  frame_count++;
  return GetWinner();
}

int GameController::GetWinner() const {
  unsigned int lost_players = 0;
  double win_time = 2.0;
  int win_player = -1;
  for (unsigned int i = 0; i < players_.size(); ++i) {
    if (players_[i]->has_won() && players_[i]->win_time() < win_time) {
      win_time = players_[i]->win_time();
      win_player = i;
    }

    if (players_[i]->has_lost()) {
      lost_players++;
    }
  }

  if (win_player != -1) {
    return win_player;
  }

  if (lost_players == players_.size() - 1) {
    for (unsigned int i = 0; i < players_.size(); ++i) {
      if (!players_[i]->has_lost()) {
        return i;
      }
    }
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

double GameController::GetFitness(unsigned int index) const {
  Player const& player = *players_.at(index);

  /* +1 for each missing checkpoint */
  double pod1_fitness = player.pods().at(0)->GetFitness(map_);
  double pod2_fitness = player.pods().at(1)->GetFitness(map_);

  return std::min(pod1_fitness, pod2_fitness);
}

bool GameController::GetNextCheckpointCollision(double& dt, Pod*& pod) {
  /* Set 2.0 as the collision tme for each pod, since we only accept 1 or less as valid. */
  std::vector<Pod*> pods;
  std::vector<double> times;
  for (auto const& player : players_) {
    for (auto const& pod : player->pods()) {
      pods.push_back(pod.get());
      times.push_back(2.0);
    }
  }

  /* Check when each pod will collide with its checkpoint */
  bool found = false;
  for (unsigned int i = 0; i < pods.size(); ++i) {
    double time;
    Pod const* p = pods[i];
    Vec2 const& checkpoint = map_.at(p->next_checkpoint());
    if (GetNextCollision(p->position(), p->velocity(), 0, checkpoint, Vec2(), 600, time)) {
      if (time < times[i] && time < 1.0) {
        times[i] = time;
        found = true;
      }
    }
  }

  if (!found) {
    return false;
  }

  /* Find the earliest pod collision. */
  dt = 2.0;
  for (unsigned int i = 0; i < pods.size(); ++i) {
    if (times[i] < dt) {
      dt = times[i];
      pod = pods[i];
    }
  }
  return true;
}

bool GameController::GetNextPlayerCollision(double& dt, Pod*& pod1, Pod*& pod2) {
  /* We need to check each pair of pods to see if they will collide, and find the earliest
   * collision. */
  std::vector<Pod*> pods;
  for (auto const& player : players_) {
    for (auto const& pod : player->pods()) {
      pods.push_back(pod.get());
    }
  }

  std::vector<std::vector<double>> times;
  for (unsigned int i = 0; i < pods.size(); ++i) {
    times.push_back(std::vector<double>());
    for (unsigned int j = 0; j < pods.size(); ++j) {
      times[i].push_back(2.0);
    }
  }

  bool found = false;
  for (unsigned int i = 0; i < pods.size() - 1; ++i) {
    for (unsigned int j = i + 1; j < pods.size(); ++j) {
      double time;
      Pod const* p1 = pods[i];
      Pod const* p2 = pods[j];

      if (GetNextCollision(p1->position(), p1->velocity(), 400, p2->position(), p2->velocity(), 400,
                           time)) {
        if (time < times[i][j] && time < 1.0) {
          times[i][j] = time;
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
  for (unsigned int i = 0; i < pods.size() - 1; ++i) {
    for (unsigned int j = i + 1; j < pods.size(); ++j) {
      if (times[i][j] < dt) {
        dt = times[i][j];
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