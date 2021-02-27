#ifndef BLOCKERFACTORY_HPP
#define BLOCKERFACTORY_HPP

#include "DualAdvancedRunner.hpp"
#include "GameServer.hpp"
#include "GeneticAlgorithm.hpp"
#include "NeuralNetwork.hpp"
#include "RunnerBlocker.hpp"

class BlockerFactory : public ISpeciesFactory<RunnerBlocker::Config> {
 public:
  static unsigned int constexpr kConfigCount = sizeof(RunnerBlocker::Config) / sizeof(double);

  RunnerBlocker::Config GenerateRandomSpecies() const override { return RunnerBlocker::Config(); }
  RunnerBlocker::Config SparseMutate(RunnerBlocker::Config const& t1) override {
    RunnerBlocker::Config config = t1;
    double* c = reinterpret_cast<double*>(&config);

    unsigned int i = std::rand() % kConfigCount;
    double change = r(0.90, 1.10);
    c[i] *= change;

    return config;
  }
  RunnerBlocker::Config CrossMutate(RunnerBlocker::Config const& t1,
                                    RunnerBlocker::Config const& t2) override {
    RunnerBlocker::Config config;
    double const* p1 = reinterpret_cast<double const*>(&t1);
    double const* p2 = reinterpret_cast<double const*>(&t2);
    double* c = reinterpret_cast<double*>(&config);

    for (unsigned int i = 0; i < kConfigCount; ++i) {
      if (std::rand() % 2 == 0) {
        c[i] = p1[i];
      } else {
        c[i] = p2[i];
      }
    }

    return SparseMutate(config);
  }
  double Evaluate(RunnerBlocker::Config& t1) override {
    NeuralNetwork runner(advanced_runner);

    static unsigned int constexpr kIterations = 50;
    double f = 0.0;
    for (unsigned int j = 0; j < kIterations; ++j) {
      DualAdvancedRunner controller1(runner);
      RunnerBlocker controller2(t1);

      GameController server;
      server.AddPlayer(controller1);
      server.AddPlayer(controller2);
      int winner = server.RunGame();
      double p0_fitness = (winner == 0) ? 0.0 : server.GetFitness(0);
      double p1_fitness = (winner == 1) ? 0.0 : server.GetFitness(1);

      f += ((1.0 + p1_fitness - p0_fitness) / 2);
    }

    f /= kIterations;
    return f;
  }

 private:
  static double r(double min = -1.0, double max = 1.0) {
    double f = (double)rand() / RAND_MAX;
    return min + f * (max - min);
  }
};

#endif