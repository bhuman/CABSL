/** 
 * The soccer root behavior.
 *
 * @author Martin Lötzsch
 */
option(play_soccer) {
  common_transition {
    /** player role is striker */
    if (role == Role::striker)
      goto striker;
    /** player role is defender */
    else if (role == Role::defender)
      goto defender;
    /** player role is midfielder */
    else if (role == Role::midfielder)
      goto midfielder;
  }

  state(striker) {
    action {
      striker();
    }
  }

  initial_state(midfielder) {
    action {
      midfielder();
    }
  }

  state(defender) {
    action {
      defender();
    }
  }
}
