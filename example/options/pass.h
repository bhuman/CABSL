/** 
 * Passes the ball to a teammate. 
 *
 * @author Martin Lötzsch
 */
option(pass) {
  initial_state(get_behind_ball) {
    transition {
      /** at ball */
      if (ball_local_direction == NW || ball_local_direction == W || ball_local_direction == SW)
        goto kick;
    }
    action {
      get_behind_ball();
    }
  }

  state(kick) {
    transition {
      /** still behind ball */
      if (ball_local_direction != N && ball_local_direction != NW && ball_local_direction != W && ball_local_direction != SW && ball_local_direction != S)
        goto get_behind_ball;
    }
    action {
      set_action({.next_action = KICK});
    }
  }
}

