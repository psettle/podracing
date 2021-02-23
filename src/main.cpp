#include <ctime>
#include "GeneticAlgorithm.hpp"
#include "NeuralNetworkFactory.hpp"
int main() {
  NeuralNetworkFactory f;
  GeneticAlgorithm<NNPair> ga(f, 100);

  for (unsigned int g = 0; g < 10000; ++g) {
    std::srand(std::time(0));
    NNPair best = ga.Generation();

    if (g % 50 == 0 && g != 0) {
      std::cout << best.Save() << std::endl;
    }
  }

  std::cout << ga.GetBest().Save() << std::endl;
  return 0;
}