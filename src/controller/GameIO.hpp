#ifndef GAMEIO_HPP
#define GAMEIO_HPP

#include <istream>
#include <string>
#include <vector>

enum class Owner { Me, Opponent };

struct PodData {
  PodData() {}

  PodData(std::istream& input, int id, Owner owner) : id(id), owner(owner) {
    input >> x >> y >> vx >> vy >> angle >> next_checkpoint_id;
  }

  int id;
  Owner owner;
  int x;
  int y;
  int vx;
  int vy;
  int angle;
  int next_checkpoint_id;
};

struct MapData {
  MapData(std::istream& input) {
    input >> laps;

    int checkpoint_count;
    input >> checkpoint_count;

    while (checkpoint_count--) {
      int x, y;
      input >> x >> y;
      checkpoints.push_back(std::pair<int, int>(x, y));
    }
  }

  int laps;
  std::vector<std::pair<int, int>> checkpoints;
};

inline void TakeMove(std::ostream& output, int x, int y, std::string const& action) {
  output << x << " " << y << " " << action << std::endl;
}

inline void TakeMove(std::ostream& output, int x, int y, double thrust) {
  TakeMove(output, x, y, std::to_string(static_cast<int>(thrust * 100)));
}

inline void TakeMoveBoost(std::ostream& output, int x, int y) { TakeMove(output, x, y, "BOOST"); }

inline void TakeMoveShield(std::ostream& output, int x, int y) { TakeMove(output, x, y, "SHIELD"); }

#endif