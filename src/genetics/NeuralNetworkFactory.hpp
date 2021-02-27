#ifndef NNFACTORY_HPP
#define NNFACTORY_HPP

#include "GeneticAlgorithm.hpp"
#include "NeuralNetwork.hpp"

class NeuralNetworkFactory : public ISpeciesFactory<NeuralNetwork> {
 public:
  NeuralNetwork GenerateRandomSpecies() const override;
  NeuralNetwork SparseMutate(NeuralNetwork const& t1) override;
  NeuralNetwork CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2) override;
  double Evaluate(NeuralNetwork& t1) override;

 private:
  static std::vector<double> Flatten(NeuralNetwork const& network);
  static void Unflatten(NeuralNetwork& network, std::vector<double> const& flat);
  NeuralNetwork GenerateRandomNetwork() const;

  static double r(double min = -1.0, double max = 1.0) {
    double f = (double)rand() / RAND_MAX;
    return min + f * (max - min);
  }
};

#endif