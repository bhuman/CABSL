/**
 * Go in a certain direction, but avoid positions already occupied.
 * This was part of a basic behavior written in C++ in the XABSL implementation.
 *
 * @author Martin Lötzsch
 * @author Thomas Röfer
 */
option(go_dir, (Action) dir) {
  common_transition {
    if (local_area[dir] != EMPTY) {
      if (dir == N && local_area[NE] == EMPTY)
        goto north_east;
      else if (dir == NE && local_area[E] == EMPTY)
        goto east;
      else if (dir == E && y < 11 && local_area[SE] == EMPTY)
        goto south_east;
      else if (dir == E && y >= 11 && local_area[NE] == EMPTY)
        goto south_east;
      else if (dir == SE && local_area[E] == EMPTY)
        goto east;
      else if (dir == S && local_area[SE] == EMPTY)
        goto south_east;
      else if (dir == SW && local_area[S] == EMPTY)
        goto south;
      else if (dir == NW && local_area[NW] == EMPTY)
        goto north;
      else
        goto do_nothing;
    }
    else
      goto is_empty;
  }

  initial_state(is_empty) {
    action {
      set_action(dir);
    }
  }
  state(do_nothing) {
    action {
      set_action(DO_NOTHING);
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

  state(east) {
    action {
      set_action(E);
    }
  }
}
