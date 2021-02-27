#include "NeuralNetworkFactory.hpp"
#include "DualAdvancedRunner.hpp"
#include "DualRunnerFinal.hpp"
#include "DualSimpleRunner.hpp"
#include "GameServer.hpp"
#include "TrainedNetworks.hpp"

NeuralNetwork NeuralNetworkFactory::GenerateRandomSpecies() const {
  NeuralNetwork network;
  NeuralNetwork::Layer layer;

  for (unsigned int i = 0; i < 24; ++i) {
    layer.bias.push_back(r());
    layer.weight.push_back(std::vector<double>());
    for (unsigned int j = 0; j < DualRunnerFinal::kInputCount; ++j) {
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

  for (unsigned int i = 0; i < DualRunnerFinal::kOutputCount; ++i) {
    layer.bias.push_back(r());
    layer.weight.push_back(std::vector<double>());
    for (unsigned int j = 0; j < 16; ++j) {
      layer.weight[i].push_back(r());
    }
  }

  network.AddLayer(layer);
  return network;
  // return NeuralNetwork(advanced_runner);
};

NeuralNetwork NeuralNetworkFactory::SparseMutate(NeuralNetwork const& t1) {
  NeuralNetwork new_nn = t1;

  for (unsigned int l = 0; l < t1.layers_.size(); ++l) {
    for (unsigned int b = 0; b < t1.layers_.at(l).bias.size(); ++b) {
      if (std::rand() % 10 == 0) {
        new_nn.layers_[l].bias[b] += r(-0.1, 0.1);
      }
    }

    for (unsigned int w_vec = 0; w_vec < t1.layers_.at(l).weight.size(); ++w_vec) {
      for (unsigned int w = 0; w < t1.layers_.at(l).weight.at(w_vec).size(); ++w) {
        if (std::rand() % 10 == 0) {
          new_nn.layers_[l].weight[w_vec][w] += r(-0.1, 0.1);
        }
      }
    }
  }

  return new_nn;
};

NeuralNetwork NeuralNetworkFactory::CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2) {
  NeuralNetwork new_nn = t1;

  auto const f1 = Flatten(t1);
  auto const f2 = Flatten(t2);
  std::vector<double> f_new;

  unsigned int crossover = std::rand() % f1.size();

  f_new.insert(f_new.end(), f1.begin(), f1.begin() + crossover);
  f_new.insert(f_new.end(), f2.begin() + crossover, f2.end());

  Unflatten(new_nn, f_new);
  return SparseMutate(new_nn);
};

double NeuralNetworkFactory::Evaluate(NeuralNetwork& t1) {
  NeuralNetwork simple(simple_runner);

  static unsigned int constexpr kIterations = 50;
  double f = 0.0;
  for (unsigned int j = 0; j < kIterations; ++j) {
    DualSimpleRunner controller1(simple);
    DualRunnerFinal controller2(t1);

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
};

std::vector<double> NeuralNetworkFactory::Flatten(NeuralNetwork const& net) {
  std::vector<double> flat;

  for (auto const& layer : net.layers_) {
    for (auto const& b : layer.bias) {
      flat.push_back(b);
    }
    for (auto const& w_vec : layer.weight) {
      for (auto const& w : w_vec) {
        flat.push_back(w);
      }
    }
  }

  return flat;
}

void NeuralNetworkFactory::Unflatten(NeuralNetwork& net, std::vector<double> const& flat) {
  unsigned int index = 0;

  for (auto& layer : net.layers_) {
    for (auto& b : layer.bias) {
      b = flat[index++];
    }
    for (auto& w_vec : layer.weight) {
      for (auto& w : w_vec) {
        w = flat[index++];
      }
    }
  }
}