#ifndef NEURALNETWORK_HPP
#define NEURALNETWORK_HPP

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

class NeuralNetwork {
 public:
  typedef std::vector<double> Activations;
  typedef std::vector<double> Bias;
  typedef std::vector<std::vector<double>> Weights;

  struct Layer {
    Weights weight;
    Bias bias;
  };

  void AddLayer(Layer const& layer);
  void SetInput(Activations const& input);
  Activations const& GetOutput() const { return output_; }

  std::string Save() const;
  void Load(std::string const& str);

  static NeuralNetwork SparseMutate(NeuralNetwork const& t1);
  static NeuralNetwork CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2);

 private:
  Activations ApplyLayer(Layer const& layer, Activations const& input);

  static inline double Relu(double a) { return std::max(0.0, a); }
  static inline double sig(double a) { return (2 * a / (1 + std::abs(a))); }

  static double fRand(double fMin, double fMax) {
    double f = (double)std::rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
  }

  std::vector<double> Flatten() const;
  void Unflatten(std::vector<double> const& flat);

  Activations output_;
  std::vector<Layer> layers_;
};

#endif