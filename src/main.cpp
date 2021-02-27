#include <ctime>
#include <fstream>
#include "BlockerConfigFactory.hpp"
#include "GeneticAlgorithm.hpp"

int main() {
  BlockerFactory f;
  GeneticAlgorithm<RunnerBlocker::Config> ga(f, 80);
  std::srand(std::time(0));
  for (unsigned int g = 0; true; ++g) {
    RunnerBlocker::Config best = ga.Generation(g);
    if (g % 10 == 0) {
      std::ofstream file("./logs/blocker_" + std::to_string(g) + "_" +
                         std::to_string(std::time(0)));
      double const* p = reinterpret_cast<double const*>(&best);

      for (unsigned int i = 0; i < BlockerFactory::kConfigCount; ++i) {
        file << p[i] << " ";
      }
      file.close();
    }
  }
  return 0;
}