/**
 * Go to a position on the field.
 * This was a basic behavior written in C++ in the XABSL implementation.
 *
 * @author Martin Lötzsch
 * @author Thomas Röfer
 */
option(go_to, args((int) x, (int) y),
              defs((int)(78) length,
                   (int)(21) width,
                   (int)((width + 1) / 2) half_width)) {
  common_transition {
    x = std::min(length, std::max(1, x));
    y = std::min(width, std::max(1, y));

    if (x < ball_x && y == ball_y)
      y += y < half_width ? 1 : -1;

    int dx = x - this->x;
    int dy = y - this->y;

    if (dy < 0) {
      if (dx >= 0 || std::abs(dx) < std::abs(dy))
        goto north_east;
      else
        goto north_west;
    }
    else if (dy > 0) {
      if (dx >= 0 || std::abs(dx) < std::abs(dy))
        goto south_east;
      else
        goto south_west;
    }
    else { // dy == 0
      if (dx > 0)
        goto east;
      else if (dx < 0)
        goto west;
      else
        goto do_nothing;
    }
  }

  initial_state(do_nothing) {
    action {
      set_action({.next_action = DO_NOTHING});
    }
  }

  state(north_east) {
    action {
      go_dir({.dir = NE});
    }
  }

  state(north_west) {
    action {
      go_dir({.dir = NW});
    }
  }

  state(south_east) {
    action {
      go_dir({.dir = SE});
    }
  }

  state(south_west) {
    action {
      go_dir({.dir = SW});
    }
  }

  state(east) {
    action {
      go_dir({.dir = E});
    }
  }

  state(west) {
    action {
      go_dir({.dir = W});
    }
  }
}
