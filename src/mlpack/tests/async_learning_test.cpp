/**
 * @file async_learning_test.hpp
 * @author Shangtong Zhang
 *
 * Test for async deep RL methods.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/core.hpp>

#include <mlpack/methods/ann/ffn.hpp>
#include <mlpack/methods/ann/init_rules/gaussian_init.hpp>
#include <mlpack/methods/ann/layer/layer.hpp>
#include <mlpack/methods/reinforcement_learning/async_learning.hpp>
#include <mlpack/methods/reinforcement_learning/environment/cart_pole.hpp>
#include <mlpack/core/optimizers/adam/adam_update.hpp>
#include <mlpack/methods/reinforcement_learning/policy/greedy_policy.hpp>
#include <mlpack/methods/reinforcement_learning/policy/aggregated_policy.hpp>
#include <mlpack/methods/reinforcement_learning/training_config.hpp>

#include <boost/test/unit_test.hpp>
#include "test_tools.hpp"

using namespace mlpack;
using namespace mlpack::ann;
using namespace mlpack::optimization;
using namespace mlpack::rl;

BOOST_AUTO_TEST_SUITE(AsyncLearningTest);

// Test async one step q-learning in Cart Pole.
BOOST_AUTO_TEST_CASE(OneStepQLearningTest)
{
  // Set up the network.
  FFN<MeanSquaredError<>, GaussianInitialization> model(MeanSquaredError<>(),
      GaussianInitialization(0, 0.001));
  model.Add<Linear<>>(4, 128);
  model.Add<ReLULayer<>>();
  model.Add<Linear<>>(128, 128);
  model.Add<ReLULayer<>>();
  model.Add<Linear<>>(128, 2);

  // Set up the policy.
  using Policy = GreedyPolicy<CartPole>;
  AggregatedPolicy<Policy> policy({Policy(0.7, 5000, 0.1),
                                   Policy(0.7, 5000, 0.01),
                                   Policy(0.7, 5000, 0.5)},
                                  arma::colvec("0.4 0.3 0.3"));

  TrainingConfig config;
  config.StepSize() = 0.0001;
  config.Discount() = 0.99;
  config.NumOfWorkers() = 16;
  config.UpdateInterval() = 6;
  config.StepLimit() = 200;
  config.TargetNetworkSyncInterval() = 200;

  OneStepQLearning<CartPole, decltype(model), VanillaUpdate, decltype(policy)>
      agent(std::move(config), std::move(model), std::move(policy));

  std::vector<double> rewards;
  size_t testEpisodes = 0;

  agent.Train([&rewards, &testEpisodes](double reward) {
    size_t maxEpisode = 50000;
    if (testEpisodes > maxEpisode)
      BOOST_REQUIRE(false);
    testEpisodes++;
    rewards.push_back(reward);
    size_t windowSize = 20;
    if (rewards.size() > windowSize)
      rewards.erase(rewards.begin());
    double avgReward = std::accumulate(
        rewards.begin(), rewards.end(), 0.0) / windowSize;
    Log::Debug << "Average return: " << avgReward
               << " Episode return: " << reward << std::endl;
    if (avgReward > 60)
      return true;
    return false;
  });
}

BOOST_AUTO_TEST_SUITE_END();
