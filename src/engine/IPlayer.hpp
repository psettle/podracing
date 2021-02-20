#ifndef IPLAYER_HPP
#define IPLAYER_HPP

#include <iostream>

class IPlayer {
 public:
  virtual void SetStreams(std::istream& input, std::ostream& output) = 0;
  virtual void Setup() = 0;
  virtual void Turn() = 0;
};

#endif