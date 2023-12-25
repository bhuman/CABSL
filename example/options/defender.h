/** 
 * Waits behind the ball.
 *
 * @author Martin Lötzsch
 */
option(defender,
       defs((int)(12) own_ball_distance,
            (int)(10) opponent_ball_distance)) {
  initial_state(own_team_has_ball) {
    transition {
      /** opponent team has ball */
      if (most_westerly_teammate_x < ball_x)
        goto opponent_team_has_ball;
    }
    action {
      go_to({.x = ball_x + own_ball_distance, .y = ball_y});
    }
  }

  state(opponent_team_has_ball) {
    transition {
      /** own team has ball */
      if (most_westerly_teammate_x >= ball_x)
        goto own_team_has_ball;
    }
    action {
      go_to({.x = ball_x + opponent_ball_distance, .y = ball_y});
    }
  }
}
