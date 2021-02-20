#ifndef INPUT_HPP
#define INPUT_HPP

#include <istream>
#include <vector>

enum class Owner { Me, Opponent };

struct PodData {
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
    std::cout << dynamic_cast<std::istringstream&>(input).str() << std::endl;
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

#endif