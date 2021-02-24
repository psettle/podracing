#ifndef NNFACTORY_HPP
#define NNFACTORY_HPP

#include "DualAdvancedRunner.hpp"
#include "DualSimpleRunner.hpp"
#include "GameServer.hpp"
#include "GeneticAlgorithm.hpp"
#include "TrainedNetworks.hpp"

typedef NeuralNetwork NeuralNetwork;

class NeuralNetworkFactory : public ISpeciesFactory<NeuralNetwork> {
 public:
  NeuralNetwork GenerateRandomSpecies() const override;
  NeuralNetwork SparseMutate(NeuralNetwork const& t1) override;
  NeuralNetwork CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2) override;
  double Evaluate(NeuralNetwork& t1) override;

 private:
  NeuralNetwork GenerateRandomNetwork() const;

  static double fRand(double fMin, double fMax) {
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
  }

  static double r() { return fRand(-1.0, 1.0); }
};

NeuralNetwork NeuralNetworkFactory::GenerateRandomSpecies() const {
  NeuralNetwork nn;

  nn = GenerateRandomNetwork();

  return nn;
};

NeuralNetwork NeuralNetworkFactory::GenerateRandomNetwork() const {
  NeuralNetwork network;
  NeuralNetwork::Layer layer;

  for (unsigned int i = 0; i < 24; ++i) {
    layer.bias.push_back(r());
    layer.weight.push_back(std::vector<double>());
    for (unsigned int j = 0; j < DualAdvancedRunner::kInputCount; ++j) {
      layer.weight[i].push_back(r());
    }
  }

  network.AddLayer(layer);
  layer.bias.clear();
  layer.weight.clear();

  for (unsigned int i = 0; i < 16; ++i) {
    layer.bias.push_back(r());
    layer.weight.push_back(std::vector<double>());
    for (unsigned int j = 0; j < 24; ++j) {
      layer.weight[i].push_back(r());
    }
  }

  network.AddLayer(layer);

  layer.bias.clear();
  layer.weight.clear();

  for (unsigned int i = 0; i < DualAdvancedRunner::kOutputCount; ++i) {
    layer.bias.push_back(r());
    layer.weight.push_back(std::vector<double>());
    for (unsigned int j = 0; j < 16; ++j) {
      layer.weight[i].push_back(r());
    }
  }

  network.AddLayer(layer);
  return network;
}

NeuralNetwork NeuralNetworkFactory::SparseMutate(NeuralNetwork const& t1) {
  NeuralNetwork p;
  p = NeuralNetwork::SparseMutate(t1);
  return p;
};

NeuralNetwork NeuralNetworkFactory::CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2) {
  NeuralNetwork p;
  p = NeuralNetwork::CrossMutate(t1, t2);
  return p;
};

double NeuralNetworkFactory::Evaluate(NeuralNetwork& t1) {
  NeuralNetwork dual_net;
  dual_net.Load(advanced_runner);

  double f = 0.0;
  for (unsigned int j = 0; j < 50; ++j) {
    DualAdvancedRunner controller1(dual_net);
    DualAdvancedRunner controller2(t1);

    GameController server;
    server.AddPlayer(controller1);
    server.AddPlayer(controller2);
    int winner = server.RunGame();
    double p0_fitness = (winner == 0) ? 0.0 : server.GetFitness(0);
    double p1_fitness = (winner == 1) ? 0.0 : server.GetFitness(1);

    f += ((1.0 + p1_fitness - p0_fitness) / 2);
  }

  f /= 50;
  return f;
};

#endif