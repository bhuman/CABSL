/** 
 * Waits in front of the other players for a pass.
 *
 * @author Martin Lötzsch
 */
option(striker) {
  initial_state(initial) {
    action {
      go_to(ball_x - 8, 11);
    }
  }
}
