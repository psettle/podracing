#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <iostream>
#include <string>

void TakeMove(std::ostream& output, int x, int y, std::string const& action) {
  output << x << " " << y << " " << action << std::endl;
}

void TakeMove(std::ostream& output, int x, int y, double thrust) {
  TakeMove(output, x, y, std::to_string(static_cast<int>(thrust * 100)));
}

void TakeMoveBoost(std::ostream& output, int x, int y) { TakeMove(output, x, y, "BOOST"); }

void TakeMoveShield(std::ostream& output, int x, int y) { TakeMove(output, x, y, "SHIELD"); }

#endif