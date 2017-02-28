/** 
 * Dribbles the ball without kicking. 
 *
 * @author Martin Lötzsch
 */
option(dribble) {
  initial_state(behind_ball) {
    transition {
      /** not behind ball */
      if (ball_local_direction != W)
        goto not_behind_ball;
      /** near opponent goal */
      else if (x < 13)
        goto behind_ball_near_opponent_goal;
    }
    action {
      set_action(W);
    }
  }

  state(behind_ball_near_opponent_goal) {
    transition {
      goto behind_ball;
    }
    action {
      set_action(KICK);
    }
  }

  state(not_behind_ball) {
    transition {
      /** not behind ball */
      if (ball_local_direction == W) {
        goto behind_ball;
      }
    }
    action {
      get_behind_ball();
    }
  }
}

