/**
 * Get behind the ball.
 * This was a basic behavior written in C++ in the XABSL implementation.
 *
 * @author Martin Lötzsch
 * @author Thomas Röfer
 */
option(get_behind_ball) {
  common_transition {
    if (ball_local_direction == E) {
      if (y < 11)
        goto south_east;
      else
        goto north_east;
    }
    else if (ball_local_direction == SE || ball_local_direction == NE || ball_local_direction == PLAYER)
      goto east;
    else if (ball_local_direction == N) {
      if (local_area[NE] == EMPTY)
        goto north_east;
      else
        goto east;
    }
    else if (ball_local_direction == S) {
      if (local_area[SE] == EMPTY)
        goto south_east;
      else
        goto east;
    }
    else if (ball_local_direction == SW)
      goto south;
    else if (ball_local_direction == NW)
      goto north;
    else if (ball_local_direction == W)
      goto west;
  }

  initial_state(use_direction) {
    action {
      set_action(ball_direction);
    }
  }

  state(north) {
    action {
      set_action(N);
    }
  }

  state(north_east) {
    action {
      set_action(NE);
    }
  }

  state(east) {
    action {
      set_action(E);
    }
  }

  state(south_east) {
    action {
      set_action(SE);
    }
  }

  state(south) {
    action {
      set_action(S);
    }
  }

  state(west) {
    action {
      set_action(W);
    }
  }
}
