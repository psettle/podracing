#ifndef GENETICALGORITHM_HPP
#define GENETICALGORITHM_HPP

#include <windows.h>
#include <algorithm>
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

  T Generation();

  T GetBest() const { return best_; };

 private:
  struct EvaluateParams {
    std::pair<unsigned int, double>* evals;
    T* candidates;
    unsigned int index;
    unsigned int size;
    ISpeciesFactory<T>* factory;
  };

  static DWORD WINAPI Evaluate(LPVOID lpParameter) {
    EvaluateParams* params = static_cast<EvaluateParams*>(lpParameter);
    for (unsigned int i = 0; i < params->size; ++i) {
      params->evals[i] = std::pair<unsigned int, double>(
          params->index + i, params->factory->Evaluate(params->candidates[i]));
    }
    return 0;
  }

  T best_;
  ISpeciesFactory<T>& factory_;
  std::vector<T> population_;
  unsigned int pop_;
};

template <class T>
GeneticAlgorithm<T>::GeneticAlgorithm(ISpeciesFactory<T>& factory, unsigned int pop)
    : best_(), factory_(factory), pop_(pop) {
  for (unsigned int i = 0; i < pop_; ++i) {
    population_.push_back(factory_.GenerateRandomSpecies());
  }
}

template <class T>

T GeneticAlgorithm<T>::Generation() {
  std::vector<std::pair<unsigned int, double>> evals(pop_);
  std::vector<HANDLE> threads;

  unsigned int pop = pop_;
  unsigned int batches = 12;
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
    params[i] = p;

    HANDLE h =
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Evaluate, (PVOID)(params.data() + i), 0, 0);
    threads.push_back(h);
  }

  for (auto& thread : threads) {
    WaitForSingleObject(thread, INFINITE);
  }
  std::srand(std::time(0));
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
      population_.push_back(survivors[0]);
    } else if (i % 2 == 0) {
      population_.push_back(factory_.CrossMutate(survivors[rand1], survivors[rand2]));
    } else {
      population_.push_back(factory_.SparseMutate(survivors[rand1]));
    }
  }

  std::cout << " best: " << evals[0].second << std::endl;
  best_ = population_[0];
  return best_;
}

#endif