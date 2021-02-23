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
    output[i] = sig(a_out);
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

NeuralNetwork NeuralNetwork::SparseMutate(NeuralNetwork const& t1) {
  NeuralNetwork new_nn = t1;

  for (unsigned int l = 0; l < t1.layers_.size(); ++l) {
    for (unsigned int b = 0; b < t1.layers_.at(l).bias.size(); ++b) {
      if (std::rand() % 5 == 0) {
        new_nn.layers_[l].bias[b] += fRand(-0.1, 0.1);
      }
    }

    for (unsigned int w_vec = 0; w_vec < t1.layers_.at(l).weight.size(); ++w_vec) {
      for (unsigned int w = 0; w < t1.layers_.at(l).weight.at(w_vec).size(); ++w) {
        if (std::rand() % 5 == 0) {
          new_nn.layers_[l].weight[w_vec][w] += fRand(-0.1, 0.1);
        }
      }
    }
  }

  return new_nn;
}

NeuralNetwork NeuralNetwork::CrossMutate(NeuralNetwork const& t1, NeuralNetwork const& t2) {
  NeuralNetwork new_nn = t1;

  auto const f1 = t1.Flatten();
  auto const f2 = t2.Flatten();
  std::vector<double> f_new;

  unsigned int crossover = std::rand() % f1.size();

  f_new.insert(f_new.end(), f1.begin(), f1.begin() + crossover);
  f_new.insert(f_new.end(), f2.begin() + crossover, f2.end());

  new_nn.Unflatten(f_new);

  return SparseMutate(new_nn);
}

std::vector<double> NeuralNetwork::Flatten() const {
  std::vector<double> flat;

  for (auto const& layer : layers_) {
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

void NeuralNetwork::Unflatten(std::vector<double> const& flat) {
  unsigned int index = 0;

  for (auto& layer : layers_) {
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