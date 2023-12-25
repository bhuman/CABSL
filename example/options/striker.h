/** 
 * Waits in front of the other players for a pass.
 *
 * @author Martin Lötzsch
 */
option(striker,
       defs((int)(8) ball_offset,
            (int)(11) half_width)) {
  initial_state(initial) {
    action {
      go_to({.x = ball_x - ball_offset, .y = half_width});
    }
  }
}
