/**
 * @file ActivationGraph
 *
 * The graph of executed options and states. Technically, the graph is a tree,
 * although options could be called more than once in an execution cycle, which
 * theoretically makes the tree an acyclic graph. However, the representation
 * is still the one of a tree.
 *
 * @author Thomas Röfer
 */

#pragma once

#include <string>
#include <vector>

namespace cabsl
{
  struct ActivationGraph
  {
    /** A node of the graph. */
    struct Node
    {
      /** Keep default constructor. */
      Node() = default;

      /**
       * Create a node.
       * @param option The name of the option.
       * @param depth The level in the call hierarchy.
       * @param state The name of the state.
       * @param optionTime How long is the option already active?
       * @param optionTime How long is the state already active?
       * @param arguments The actual arguments of the option as strings.
       */
      Node(const std::string& option, int depth,
           const std::string& state, int optionTime,
           int stateTime, const std::vector<std::string>& arguments) :
      option(option),
      depth(depth),
      state(state),
      optionTime(optionTime),
      stateTime(stateTime),
      arguments(arguments)
      {
      }

      std::string option; /**< The name of the option. */
      int depth = 0; /**< The level in the call hierarchy. */
      std::string state; /**< The name of the state. */
      int optionTime = 0; /**< How long is the option already active? */
      int stateTime = 0; /**< How long is the state already active? */
      std::vector<std::string> arguments; /**< The actual arguments of the option as strings. */
    };

    /** The constructor reserves some nodes in the graph. */
    ActivationGraph()
    {
      graph.reserve(100);
    }

    std::vector<Node> graph; /**< The nodes of the graph. */
  };
}
