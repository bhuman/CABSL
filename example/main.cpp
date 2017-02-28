/**
 * This file implements the main class of the CABSL Example Agents that
 * is required by ascii soccer. It simply instantiates four instances of
 * the behavior and lets each player execute its instance of it.
 *
 * @author Thomas Röfer
 */

extern "C" {
#include <players.h>
}
#include "behavior.h"

/*
players.h includes a rather unusual macro, UN().  This macro
will convert the names of the functions in this .c file to
unique names.  For instance, if this were the east team, UN(team_name)()
will become EASTteam_name().  This is necessary for the automatic
compiling and linking of arbitrary teams for tournaments.

Below are the functions player1() through player4().  Each
function is passed 4 parameters.  The incoming parameters are 
the robot's sensory information:  

	local_area[]	Reveals what's nearby.  Indexed by
			N,NE,E,SE,S, etc. so that, for example,
			local_area[S] tells you what is in the
			cell to the south of the robot.  Possible 
			values include: BOUNDARY, OPPONENT, TEAMMATE,
			BALL and EMPTY.

	ball_direction	Compass heading to the ball: N, S, E, W, NE, etc.

	x, y		The robot's location.  y varies from 1 to 22,
			x varies from 1 to 78.

The function should return an integer indicating either a direction to
move (N, NE, E, S, etc) or KICK to kick the ball.
*/

static Behavior behaviors[4] = {Behavior(0), Behavior(1), Behavior(2), Behavior(3)};

/*-----------------------------------------------------

	player1()

-----------------------------------------------------*/
int UN(player1)(int local_area[9], int ball_direction, int x, int y)
{
  return behaviors[0].execute(local_area, static_cast<Behavior::Action>(ball_direction), x, y);
}

/*-----------------------------------------------------

	player2()

-----------------------------------------------------*/
int UN(player2)(int local_area[9], int ball_direction, int x, int y)
{
  return behaviors[1].execute(local_area, static_cast<Behavior::Action>(ball_direction), x, y);
}

/*-----------------------------------------------------

	player3()

-----------------------------------------------------*/
int UN(player3)(int local_area[9], int ball_direction, int x, int y)
{
  return behaviors[2].execute(local_area, static_cast<Behavior::Action>(ball_direction), x, y);
}

/*-----------------------------------------------------

	player4()

-----------------------------------------------------*/
int UN(player4)(int local_area[9], int ball_direction, int x, int y)
{
  return behaviors[3].execute(local_area, static_cast<Behavior::Action>(ball_direction), x, y);
}

/*-----------------------------------------------------

	team_name()

	Every team must have a name.  Make sure it
	is exactly 20 characters.  Pad with spaces.

-----------------------------------------------------*/
char *UN(team_name)()
{
  const char	*s;

  /*  "####################\0" <--- 20 characters */
  s = "CABSL Example Agents\0";
  return(const_cast<char*>(s));
}

/*-----------------------------------------------------

	initialize_game()

	This function is called only once per
	game, before play begins.  You can leave it
	empty, or put code here to initialize your
	variables, etc.

-----------------------------------------------------*/
void UN(initialize_game)()
{
}

/*-----------------------------------------------------

	initialize_point()

	This function is called once per point, just
	before play begins for that point.
	You can leave it empty, or put code here to initialize 
	your variables, etc.

-----------------------------------------------------*/
void UN(initialize_point)()
{
}

/*-----------------------------------------------------

	lost_point()

	If your team loses a point, this function is
	called, otherwise, if you win the point, won_point()
	is called.  You can leave it empty, or put code here 
	for negative re-inforcement, etc.

-----------------------------------------------------*/
void UN(lost_point)()
{
}

/*-----------------------------------------------------

	won_point()

	If your team wins a point, this function is
	called, otherwise, if you lose the point, lost_point()
	is called.  You can leave it empty, or put code here 
	for positive re-inforcement, etc.

-----------------------------------------------------*/
void UN(won_point)()
{
}

/*-----------------------------------------------------

        game_over()

        This function is called once at the end of
        the match.  You can leave it empty, or put code 
        here to save things to a file, etc.

-----------------------------------------------------*/
void UN(game_over)()
{
}
