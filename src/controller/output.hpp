#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <iostream>
#include <string>

inline void TakeMove(std::ostream& output, int x, int y, std::string const& action) {
  output << x << " " << y << " " << action << std::endl;
}

inline void TakeMove(std::ostream& output, int x, int y, double thrust) {
  TakeMove(output, x, y, std::to_string(static_cast<int>(thrust * 100)));
}

inline void TakeMoveBoost(std::ostream& output, int x, int y) { TakeMove(output, x, y, "BOOST"); }

inline void TakeMoveShield(std::ostream& output, int x, int y) { TakeMove(output, x, y, "SHIELD"); }

#endif