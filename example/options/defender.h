/** 
 * Waits behind the ball.
 *
 * @author Martin Lötzsch
 */
option(defender) {
  initial_state(own_team_has_ball) {
    transition {
      /** opponent team has ball */
      if (most_westerly_teammate_x < ball_x)
        goto opponent_team_has_ball;
    }
    action {
      go_to(ball_x + 12, ball_y);
    }
  }

  state(opponent_team_has_ball) {
    transition {
      /** own team has ball */
      if (most_westerly_teammate_x >= ball_x)
        goto own_team_has_ball;
    }
    action {
      go_to(ball_x + 10, ball_y);
    }
  }
}
