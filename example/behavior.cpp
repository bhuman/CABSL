/**
 * This file implements the actual behavior of the CABSL Example Agent.
 * The implementation is based on the XABSL Example Agent that can
 * be downloaded from the XABSL website (www.xabsl.de). The XABSL
 * Example agent was originally developed by Martin Lötzsch. The
 * implementation here is a compact translation of Martin Lötzsch
 * work into the formalism of CABSL. In many aspects, it tries to be
 * as similar as possible to the original implementation, which also
 * includes the coding style. Therefore it is a little bit off from
 * the style used for CABSL itself.
 *
 * One major difference are that the basic behaviors from the original
 * implementation are also written as options.
 *
 * @author Thomas Röfer
 */

#include <cmath>
#include "behavior.h"

int Behavior::ball_x = 0;
int Behavior::ball_y = 0;
int Behavior::team_x[4];
int Behavior::team_y[4];
Behavior::Action Behavior::team_ball_direction[4];

Behavior::Action Behavior::execute(int local_area[9], Action ball_direction, int x, int y) {

  // copy arguments
  std::memcpy(this->local_area, local_area, sizeof(this->local_area));
  this->ball_direction = team_ball_direction[player_number] = ball_direction;
  this->x = team_x[player_number] = x;
  this->y = team_y[player_number] = y;

  updateWorldState();

  // execute behavior
  beginFrame(frame_counter++);
  Cabsl<Behavior>::execute("play_soccer");
  endFrame();

  showActivationGraph();

  return next_action;
}

void Behavior::updateWorldState()
{
  // compute ball_local_direction
  ball_local_direction = DO_NOTHING;
  for (int i = 0; i < 9; ++i)
    if (local_area[i] == BALL)
      ball_local_direction = static_cast<Action>(i);

  // estimate the ball position: fill "field" with the number of players that see the ball for each field element
  char field[MAX_X][MAX_Y] = {0};

  double temp_angle;
  int temp_dir;
  for (int i = 1; i < MAX_Y - 1; ++i)
    for (int j = 1; j < MAX_X - 1; ++j)
      for (int k = 3; k >= 0; --k) {
      if (j == team_x[k] && i == team_y[k])
        temp_angle = 0;
      else
        temp_angle = std::atan2(j - team_x[k], i - team_y[k]);
      temp_angle += PI;
      temp_angle = 360 * temp_angle / (2 * PI);
      temp_dir = N;
      if (temp_angle > 22.5 + 0 * 45) temp_dir = NW;
      if (temp_angle > 22.5 + 1 * 45) temp_dir = W;
      if (temp_angle > 22.5 + 2 * 45) temp_dir = SW;
      if (temp_angle > 22.5 + 3 * 45) temp_dir = S;
      if (temp_angle > 22.5 + 4 * 45) temp_dir = SE;
      if (temp_angle > 22.5 + 5 * 45) temp_dir = E;
      if (temp_angle > 22.5 + 6 * 45) temp_dir = NE;
      if (temp_angle > 22.5 + 7 * 45) temp_dir = N;

      if (temp_dir == team_ball_direction[k])
        field[j][i] += 1;
    }

  // estimate the ball position: search for peaks
  int sum_x = 0, sum_y = 0, num_fields = 0;

  for (int i = 1; i < MAX_Y - 1; ++i)
    for (int j = 1; j < MAX_X - 1; ++j)
      if (field[j][i] == 4) {
        sum_x += j;
        sum_y += i;
        ++num_fields;
      }

  if (num_fields != 0) {
    ball_x = sum_x / num_fields;
    ball_y = sum_y / num_fields;
  }
  else {
    for (int i = 1; i < MAX_Y - 1; ++i)
      for (int j = 1; j < MAX_X - 1; ++j)
        if (field[j][i] == 3) {
          sum_x += j;
          sum_y += i;
          ++num_fields;
        }
    if (num_fields != 0) {
      ball_x = sum_x / num_fields;
      ball_y = sum_y / num_fields;
    }
  }

  if (local_area[N] == BALL) { ball_x = x; ball_y = y - 1; }
  if (local_area[NE] == BALL) { ball_x = x + 1; ball_y = y - 1; }
  if (local_area[E] == BALL) { ball_x = x + 1; ball_y = y; }
  if (local_area[SE] == BALL) { ball_x = x +  1; ball_y = y + 1; }
  if (local_area[S] == BALL) { ball_x = x ; ball_y = y + 1; }
  if (local_area[SW] == BALL) { ball_x = x - 1; ball_y = y + 1; }
  if (local_area[W] == BALL) { ball_x = x - 1; ball_y = y; }
  if (local_area[NW] == BALL) { ball_x = x - 1; ball_y = y - 1; }

  // compute ball distance to estimate ball position
  if(ball_local_direction != DO_NOTHING)
    ball_distance = 1.f;
  else
    ball_distance = std::sqrt(pow(ball_x - x, 2) + std::pow(ball_y - y, 2));

  // compute most_westerly_teammate_x
  most_westerly_teammate_x = 78;
  for (int x : team_x)
    if (x < most_westerly_teammate_x)
      most_westerly_teammate_x = x;

  // compute role
  double ball_distances[4];
  int rank[4] = {0, 1, 2, 3};

  for (int i = 0; i < 4; ++i)
    ball_distances[i] = std::sqrt(std::pow(team_x[i] - ball_x, 2) + std::pow(team_y[i] - ball_y, 2));

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (ball_distances[rank[j]] > ball_distances[rank[j + 1]])
      {
        int temp = rank[j + 1];
        rank[j + 1] = rank[j];
        rank[j] = temp;
      }

  for (int i = 0; i < 4; ++i)
    if (rank[i] == player_number)
      role = i < 2 ? Role::midfielder
                   : i == 2 ? (x >= team_x[3] ? Role::defender : Role::striker)
                            : (x > team_x[2] ? Role::striker : Role::defender);

  if (ball_distances[player_number] < 3 || team_x[player_number] > 73)
    role = Role::midfielder;
}

void Behavior::showActivationGraph()
{
  if (!window)
    window = subwin(stdscr, 14, 39, 25 + player_number / 2 * 15, player_number % 2 * 41);
  wclear(window);

  int y = 0;
  for(const cabsl::ActivationGraph::Node& activeOption : activationGraph.graph)
  {
    mvwprintw(window, y, activeOption.depth - 1, "%s", activeOption.option.c_str());
    mvwprintw(window, y++, 35, "%4d", activeOption.optionTime);

    for(const std::string& argument : activeOption.arguments)
      mvwprintw(window, y++, activeOption.depth + 1, "%s", argument.c_str());

    mvwprintw(window, y, activeOption.depth + 1, "state = %s", activeOption.state.c_str());
    mvwprintw(window, y++, 35, "%4d", activeOption.stateTime);
  }
  wrefresh(window);
}
