#include "NeuralNetwork.hpp"
#include <limits>
#include <sstream>

void NeuralNetwork::AddLayer(Layer const& layer) { layers_.push_back(layer); }

void NeuralNetwork::SetInput(Activations const& input) {
  output_ = input;
  for (auto const& layer : layers_) {
    output_ = ApplyLayer(layer, output_);
  }
}

NeuralNetwork::Activations NeuralNetwork::ApplyLayer(Layer const& layer, Activations const& input) {
  Activations output(layer.weight.size());

  for (unsigned int i = 0; i < layer.weight.size(); ++i) {
    std::vector<double> const& weight = layer.weight[i];
    double a_out = -layer.bias[i];
    for (unsigned int j = 0; j < weight.size(); ++j) {
      a_out += input[j] * weight[j];
    }
    output[i] = FastSigmoid(a_out);
  }

  return output;
}

std::string NeuralNetwork::Save() const {
  std::ostringstream save;
  save.precision(std::numeric_limits<double>::max_digits10);
  save << layers_.size() << " ";
  for (auto const& layer : layers_) {
    save << layer.weight.size() << " " << layer.weight[0].size() << " ";
    for (auto const& b : layer.bias) {
      save << b << " ";
    }
    for (auto const& w_vec : layer.weight) {
      for (auto const& w : w_vec) {
        save << w << " ";
      }
    }
  }

  return save.str();
}
void NeuralNetwork::Load(std::string const& str) {
  std::istringstream load(str);
  int n_layers;
  load >> n_layers;
  layers_ = std::vector<Layer>(n_layers);
  for (auto& layer : layers_) {
    int x, y;
    load >> x >> y;
    layer.bias = Bias(x);
    layer.weight = Weights(x);
    for (auto& b : layer.bias) {
      load >> b;
    }
    for (auto& w_vec : layer.weight) {
      w_vec = std::vector<double>(y);
      for (auto& w : w_vec) {
        load >> w;
      }
    }
  }
}