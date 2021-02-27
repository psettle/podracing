#ifndef GENETICALGORITHM_HPP
#define GENETICALGORITHM_HPP

#include <windows.h>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <vector>

template <class T>
class ISpeciesFactory {
 public:
  virtual T GenerateRandomSpecies() const = 0;
  virtual T SparseMutate(T const& t1) = 0;
  virtual T CrossMutate(T const& t1, T const& t2) = 0;
  virtual double Evaluate(T& t1) = 0;
};

template <class T>
class GeneticAlgorithm {
 public:
  GeneticAlgorithm(ISpeciesFactory<T>& factory, unsigned int pop = 1000);

  T Generation(unsigned int generation_index);

 private:
  struct EvaluateParams {
    std::pair<unsigned int, double>* evals;
    T* candidates;
    unsigned int index;
    unsigned int size;
    ISpeciesFactory<T>* factory;
    unsigned int seed;
  };

  static DWORD WINAPI Evaluate(LPVOID lpParameter) {
    EvaluateParams* params = static_cast<EvaluateParams*>(lpParameter);
    for (unsigned int i = 0; i < params->size; ++i) {
      std::srand(params->seed);
      params->evals[i] = std::pair<unsigned int, double>(
          params->index + i, params->factory->Evaluate(params->candidates[i]));
    }
    return 0;
  }

  ISpeciesFactory<T>& factory_;
  std::vector<T> population_;
  unsigned int pop_;
};

template <class T>
GeneticAlgorithm<T>::GeneticAlgorithm(ISpeciesFactory<T>& factory, unsigned int pop)
    : factory_(factory), pop_(pop) {
  for (unsigned int i = 0; i < pop_; ++i) {
    population_.push_back(factory_.GenerateRandomSpecies());
  }
}

static unsigned int constexpr kSavedPeaks = 1;
template <class T>
T GeneticAlgorithm<T>::Generation(unsigned int generation_index) {
  std::vector<std::pair<unsigned int, double>> evals(pop_);
  std::vector<HANDLE> threads;

  unsigned int pop = pop_;
  unsigned int batches = 16;
  std::vector<EvaluateParams> params(batches);

  for (unsigned int i = 0; i < batches; ++i) {
    unsigned int division = batches - 1;
    unsigned int index = i * (pop_ / division);
    unsigned int batch_size = std::min(pop, pop_ / division);
    pop -= std::min(pop_ / division, pop);

    EvaluateParams p;
    p.evals = &evals[index];
    p.candidates = &population_[index];
    p.size = batch_size;
    p.factory = &factory_;
    p.index = index;
    p.seed = std::time(0);
    params[i] = p;

    HANDLE h =
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Evaluate, (PVOID)(params.data() + i), 0, 0);
    threads.push_back(h);
  }

  for (auto& thread : threads) {
    WaitForSingleObject(thread, INFINITE);
  }

  std::sort(
      evals.begin(), evals.end(),
      [](std::pair<unsigned int, double> const& left,
         std::pair<unsigned int, double> const& right) { return left.second < right.second; });

  std::vector<T> survivors;
  for (unsigned int i = 0; i < pop_ / 5; ++i) {
    unsigned int survivor_index = evals[i].first;
    survivors.push_back(population_[survivor_index]);
  }

  population_.clear();
  for (unsigned int i = 0; i < pop_; ++i) {
    unsigned int rand1 = std::rand() % survivors.size();
    unsigned int rand2 = std::rand() % survivors.size();
    if (i == 0) {
      population_.push_back(survivors[i]);
    } else {
      population_.push_back(factory_.CrossMutate(survivors[rand1], survivors[rand2]));
    }
  }

  std::cout << "gen" << generation_index << " best: ";
  for (unsigned int i = 0; i < 10; ++i) {
    std::cout << evals[i].second << " ";
  }
  std::cout << std::endl;

  return population_[0];
}

#endif