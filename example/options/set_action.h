/**
 * Set the next action to perform.
 *
 * @author Thomas Röfer
 */
option(set_action, (Action) next_action) {
  initial_state(set) {
    action {
      this->next_action = next_action;
    }
  }
}
