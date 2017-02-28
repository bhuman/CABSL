/**
 * @file ActivationGraph
 *
 * The graph of executed options and states.
 *
 * @author <a href="mailto:Thomas.Roefer@dfki.de">Thomas Röfer</a>
 */

#pragma once

#include <string>
#include <vector>

struct ActivationGraph
{
  struct Node
  {
    Node() = default;
    Node(const std::string& option, int depth,
         const std::string& state, int optionTime,
         int stateTime, const std::vector<std::string>& parameters) :
      option(option),
      depth(depth),
      state(state),
      optionTime(optionTime),
      stateTime(stateTime),
      parameters(parameters)
    {
    }

    std::string option;
    int depth = 0;
    std::string state;
    int optionTime = 0;
    int stateTime = 0;
    std::vector<std::string> parameters;
  };

  ActivationGraph()
  {
    graph.reserve(100);
  }

  std::vector<Node> graph;
};
