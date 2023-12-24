/** 
 * Waits in front of the other players for a pass.
 *
 * @author Martin Lötzsch
 */
option(striker) {
  initial_state(initial) {
    action {
      go_to({.x = ball_x - 8, .y = 11});
    }
  }
}
