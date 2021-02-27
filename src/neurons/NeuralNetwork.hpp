#ifndef NEURALNETWORK_HPP
#define NEURALNETWORK_HPP

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

class NeuralNetwork {
 public:
  friend class NeuralNetworkFactory;
  typedef std::vector<double> Activations;
  typedef std::vector<double> Bias;
  typedef std::vector<std::vector<double>> Weights;

  struct Layer {
    Weights weight;
    Bias bias;
  };

  NeuralNetwork() {}
  NeuralNetwork(std::string const& description) { Load(description); }

  void AddLayer(Layer const& layer);
  void SetInput(Activations const& input);
  Activations const& GetOutput() const { return output_; }
  std::string Save() const;

 private:
  void Load(std::string const& str);

  Activations ApplyLayer(Layer const& layer, Activations const& input);

  static double FastSigmoid(double a) { return (2 * a / (1 + std::abs(a))); }

  Activations output_;
  std::vector<Layer> layers_;
};

#endif