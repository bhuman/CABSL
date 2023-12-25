/** 
 * Handles the ball. 
 *
 * @author Martin Lötzsch
 */
option(midfielder,
       defs((int)(3) close_ball_distance,
            (int)(2) pass_threshold)) {
  initial_state(get_to_ball) {
    transition {
      /** close to ball */
      if (ball_distance <= close_ball_distance) {
        /** no teammate to the west */
        if (most_westerly_teammate_x > ball_x + pass_threshold)
          goto dribble;
        else
          goto pass;
      }
    }
    action {
      go_to({.x = ball_x, .y = ball_y});
    }
  }

  state(pass) {
    transition {
      /** far from ball */
      if (ball_distance > close_ball_distance)
        goto get_to_ball;
    }
    action {
      pass();
    }
  }

  state(dribble) {
    transition {
      /** far from ball */
      if (ball_distance > close_ball_distance)
        goto get_to_ball;
    }
    action {
      dribble();
    }
  }
}

