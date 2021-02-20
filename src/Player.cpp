#include "Player.hpp"

Player::Player(IPlayer& controller)
    : controller_(controller), output_(), input_(), timeout_(100), boosts_available_(1) {
  controller_.SetStreams(input_, output_);
  pods_.push_back(Pod());
  pods_.push_back(Pod());
}

void Player::Setup(std::string const& data) {
  input_.clear();
  input_.str(data);
  controller_.Setup();
}

void Player::SetInitialTurnConditions(std::string const& input_data) {
  std::vector<PodControl> control = CollectBotOutput(input_data);

  for (unsigned int i = 0; i < control.size(); ++i) {
    pods_[i].SetTurnConditions(control[i], boosts_available_);
  }
}

void Player::GetGameInput(std::ostringstream& game_input) {
  for (auto const& pod : pods_) {
    pod.WritePodState(game_input);
  }
}

void Player::EndTurn() {
  bool progress = false;
  for (auto& pod : pods_) {
    if (pod.made_progress()) {
      progress = true;
      if (pod.has_won()) {
        has_won_ = true;
        win_time_ = pod.progress_time();
      }
    }
    pod.EndTurn();
  }

  if (progress) {
    timeout_ = 100;
  } else {
    if (timeout_ > 0) {
      timeout_--;
    } else {
      has_lost_ = true;
    }
  }
}

void Player::AdvancePods(double dt) {
  for (auto& pod : pods_) {
    pod.Advance(dt);
  }
}

std::vector<PodControl> Player::CollectBotOutput(std::string const& input_data) {
  input_.clear();
  output_.str("");
  output_.clear();
  input_.str(input_data);
  controller_.Turn();

  std::istringstream actions(output_.str());
  std::vector<PodControl> output;
  for (unsigned int i = 0; i < pods_.size(); ++i) {
    PodControl control;
    actions >> control.x >> control.y >> control.action;
    output.push_back(control);
  }

  return output;
}

Pod& Player::pod(unsigned int index) { return pods_[index]; }
Pod const& Player::pod(unsigned int index) const { return pods_.at(index); }