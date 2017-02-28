/** 
 * Handles the ball. 
 *
 * @author Martin Lötzsch
 */
option(midfielder) {
  initial_state(get_to_ball) {
    transition {
      /** close to ball */
      if (ball_distance <= 3) {
        /** no teammate to the west */
        if (most_westerly_teammate_x > ball_x + 2)
          goto dribble;
        else
          goto pass;
      }
    }
    action {
      go_to(ball_x, ball_y);
    }
  }

  state(pass) {
    transition {
      /** far from ball */
      if (ball_distance > 3)
        goto get_to_ball;
    }
    action {
      pass();
    }
  }

  state(dribble) {
    transition {
      /** far from ball */
      if (ball_distance > 3)
        goto get_to_ball;
    }
    action {
      dribble();
    }
  }
}

