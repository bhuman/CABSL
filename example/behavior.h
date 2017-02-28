/**
 * This file declares the actual behavior of the CABSL Example Agent.
 * The implementation is based on the XABSL Example Agent that can
 * be downloaded from the XABSL website (www.xabsl.de). The XABSL
 * Example agent was originally developed by Martin Lötzsch. The
 * implementation here is a compact translation of Martin Lötzsch
 * work into the formalism of CABSL. In many aspects, it tries to be
 * as similar as possible to the original implementation, which also
 * includes the coding style. Therefore it is a little bit off from
 * the style used for CABSL itself.
 * 
 * One major difference are that the basic behaviors from the original
 * implementation are also written as options.
 *
 * @author Thomas Röfer
 */

#include <curses.h>
#include <soccer.h>
#include <Cabsl.h>

// All actions are undefined to define an enum instead (not necessary, but nicer).
#undef NW
#undef N
#undef NE
#undef W
#undef PLAYER
#undef E
#undef SW
#undef S
#undef SE
#undef KICK
#undef DO_NOTHING

class Behavior : public Cabsl<Behavior> {
public:
  /**
   * All actions that can be performed. The first nine actions are actually
   * directions. Therefore, this type is also used to represent directions.
   */
  enum Action {
    NW, N, NE, W, PLAYER, E, SW, S, SE, KICK, DO_NOTHING
  };

private:
  int local_area[9]; /**< The local area as passed by ascii soccer. */
  Action ball_direction; /**< The ball direction as passed by ascii soccer. */
  int x; /**< The player's x coordinate as passed by ascii soccer. */
  int y; /**< The player's y coordinate as passed by ascii soccer. */

  static int ball_x; /**< A shared estimate of the ball's x coordinate. */
  static int ball_y; /**< A shared estimate of the ball's y coordinate. */
  double ball_distance; /**< The players distance to the estimated ball. */
  Action ball_local_direction; /**< The direction to the ball if it is in the local area. Otherwise DO_NOTHING. */
  int most_westerly_teammate_x; /**< The x coordinate of the most west player. */
  enum class Role {defender, midfielder, striker} role; /**< The role of the player. */
  Action next_action; /**< The next action is set here. */

#include "options.h" // Include all options into the body of this class.

  // The following members are helpers not directly used by the behavior.
  unsigned frame_counter = 0; /**< Frame counter. Is increased in each frame. */
  int player_number; /**< The number of this player [0..3]. */
  static Action team_ball_direction[4]; /**< The shared ball directions of all players. */
  static int team_x[4]; /**< The shared x coordinates of all players. */
  static int team_y[4]; /**< The shared y coordinates of all players. */
  ActivationGraph activationGraph; /**< The activation graph used for debugging. */
  WINDOW* window = nullptr; /**< The window in which the activation graph is shown. */

  /** Update the world state, i.e. the input symbols. */
  void updateWorldState();
  
  /** Shows the activation graph below the field. */
  void showActivationGraph();

public:
  /**
   * Create a new behavior.
   * @param player_number The number of this player [0..3].
   */
  Behavior(int player_number)
  : Cabsl<Behavior>(&activationGraph),
    player_number(player_number) {}

  /**
   * Execute a single behavior step.
   * @param local_area The local area as passed by ascii soccer.
   * @param ball_direction The ball direction as passed by ascii soccer.
   * @param x The player's x coordinate as passed by ascii soccer.
   * @param y The player's y coordinate as passed by ascii soccer.
   * @return The next action to perform.
   */
  Action execute(int local_area[9], Action ball_direction, int x, int y);
};

/**
 * Allow parameters of type Action to be shown as text.
 * @param stream The output stream that is written to.
 * @param a The action to write.
 * @return The output stream that is written to.
 */
inline std::ostream& operator<<(std::ostream& stream, Behavior::Action a) {
  static const char* names[] = {
    "NW", "N", "NE", "W", "PLAYER", "E", "SW", "S", "SE", "KICK", "DO_NOTHING"
  };
  return stream << names[a];
}
