/*============================================================================

	rollers.c


	Steve Rowe		cerebus@crl.com

     This team uses a position-based strategy, with dynamic assessment of
     which team member takes each position.  The positions are as follows:


                                  < (northWing)



                       < (lead)
                        < (support)



                                  < (southWing)


===========================================================================*/

#include <stdlib.h>
#include "players.h"

/*----------------------------------------------------------------------------
   The WINGSPAN parameter is the radius of the diamond formation.
----------------------------------------------------------------------------*/
static int WINGSPAN;

/*----------------------------------------------------------------------------
    The CONTROL_TIME parameter is the number of turns that the players will
    stay in formation after the lead has lost contact with the ball.  If
    this value is too small, then the players will never get into position,
    so there may as well not BE positions.  If the value is too big, then
    the players will take too long to decide to start looking for the ball.

    The value used for the tournament is 13, which was determined to give
    the best win/loss ratio in 100 point samples played against WALL.

    Each opponent would theoretically have different "best" values for this
    parameter, but I will leave the heuristic approach for next year's
    tournement.
----------------------------------------------------------------------------*/
static int CONTROL_TIME;

static int leader;  /* ID number of the current lead (0 to 3) */
static int plx[4];  /* Last known X coordinate of each player */
static int ply[4];  /* Last known Y coordinate of each player */

static int haveBall = 0;  /* Down-counter indicating ball ownership */

/*  Some useful type definitions   */
typedef int PlayerFn(int la[9], int bd, int x, int y);
typedef int (*PlayerFnPtr)(int la[9], int bd, int x, int y);

/*----------------------------------------------------------------------------
    Function prototypes.  Since I developed this code on machines with ANSI-
    only compilers, and I prefer strict type checking, I provided these.
----------------------------------------------------------------------------*/
PlayerFn UN(player1);
PlayerFn UN(player2);
PlayerFn UN(player3);
PlayerFn UN(player4);

PlayerFn UN(lead);       /* Behaviour to be used by the leader */
PlayerFn UN(northWing);  /* Behaviour to be used by the north wing */
PlayerFn UN(southWing);  /* Behaviour to be used by the south wing */
PlayerFn UN(rear);       /* Behaviour to be used by the rear */

/*----------------------------------------------------------------------------
   Each player function simply calls the behave function with its player ID.
   It is up to the behave function to call the appropriate behaviour function
   for each player based on the ID.
----------------------------------------------------------------------------*/
int UN(behave)(int id, int la[9], int bd, int x, int y);


/*----------------------------------------------------------------------------
   roles[] is an array of pointers to behavior functions.  There is one entry
   in the array for each player on the team.  The contents of this array are
   chosen dynamically in the regroup function.
----------------------------------------------------------------------------*/
static PlayerFnPtr roles[4];


/*----------------------------------------------------------------------------
    The near_object function returns the direction ID of a given object
    within a given local_area.  If there is no such object in the local
    area, 9 is returned.
----------------------------------------------------------------------------*/
int UN(near_object)(int la[9], int obj)
{
    int i;

    for (i = 0; i < 9; ++i)
    {
        if (la[i] == obj) break;
    }

    return i;
}

/*=============================================================================
     The player functions are basically stubs, since all players do the same
     thing, which is to call their behaviour functions through the roles
     array.
=============================================================================*/

int UN(player1)(int la[9], int bd, int x, int y)
{
    return UN(behave)(0, la, bd, x, y);
}

int UN(player2)(int la[9], int bd, int x, int y)
{
    return UN(behave)(1, la, bd, x, y);
}

int UN(player3)(int la[9], int bd, int x, int y)
{
    return UN(behave)(2, la, bd, x, y);
}

int UN(player4)(int la[9], int bd, int x, int y)
{
    return UN(behave)(3, la, bd, x, y);
}

/*----------------------------------------------------------------------------
   The regroup function is called whenever a player makes first contact
   with the east side of the ball.  It redesignates that player as the lead,
   and reassigns the other players' roles based on their position relative
   to the new leader.
----------------------------------------------------------------------------*/
void UN(regroup)(int newLead)
{
    int i;
    int score, good = 0;

    /* Zero out all the current roles */
    for (i = 0; i < 4; ++i)
    {
        roles[i] = 0;
    }

    /* Assign lead role to given newLead */
    leader = newLead;
    roles[newLead] = UN(lead);

    /* find south wing */
    score = 0;
    for (i = 0; i < 4; ++i)
    {
        if (roles[i]) continue;
        if (ply[i] > score)
        {
            score = ply[i];
            good = i;
        }
    }

    roles[good] = UN(southWing);

    /* find north wing */
    score = 90;
    for (i = 0; i < 4; ++i)
    {
        if (roles[i]) continue;
        if (ply[i] < score)
        {
            score = ply[i];
            good = i;
        }
    }

    roles[good] = UN(northWing);

    /* Find rear */
    score = 0;
    for (i = 0; i < 4; ++i)
    {
        if (roles[i]) continue; /* If this player already has a role, skip */
        if (plx[i] > score)
        {
            score = plx[i];  /* keep player with largest X (farthest back) */
            good = i;
        }
    }

    roles[good] = UN(rear);
}

/*============================================================================
    Here is the cool function, which just returns a behaviour based on the
    ID passed in as the first parameter.
============================================================================*/

int UN(behave)(int id, int la[9], int bd, int x, int y)
{
    /* Update the "where I am" array */
    plx[id] = x;  ply[id] = y;

    /* At the beginning of each turn, decrement the downcounter */
    if (id == 0) --haveBall;

    /* If this player is on the business end of the ball, then assign him
       as leader. */
    if ((UN(near_object)(la, BALL) < 9) &&
        (bd != E) && (bd != NE) && (bd != SE) && (bd != N) && (bd != S))
    {
        haveBall = CONTROL_TIME;  /* Reset down-counter for possession */

        if (leader != id)
        {
            /* We have a new leader.  Regroup! */
            UN(regroup(id));
        }
    }
    else
    {
        /* If the downcounter has expired, make leader invalid. */
        if (haveBall <= 0) leader = -1;
    }

    if (leader >= 0)
    {
        /* If we have a leader, then perform your given duty. */
        return (*roles[id])(la, bd, x, y);
    }
    else
    {
        /* If we have NO leader, then it's every bot for itself. */
        return UN(lead)(la, bd, x, y);
    }
}

/*----------------------------------------------------------------------------

                                  LEAD()

    The leader is by far the most complex of the behaviour functions, since
    it is always the behaviour used by the current ball handler.  The rules
    used to decide how to handle various situations was determined primarily
    by impiracal methods, secondarily by careful design.  Given more time,
    I would have used a genetic algorithm engine to find the optimal rules
    for all situations.  Maybe next year.

----------------------------------------------------------------------------*/
int UN(lead)(int local_area[9], int ball_direction, int x, int y)
{
    int i;                       /* Generic loop index */
    int kickSouth = 0;  /* Counter used to determine whether to kick N or S */

    /*
       If I'm on the east side of the ball and there is an opponent, then
       don't mess around.  Kick the ball somewhere else.
    */
    if ((UN(near_object)(local_area, OPPONENT) < 9) &&
          (local_area[SW] == BALL)) return(KICK);
    if ((UN(near_object)(local_area, OPPONENT) < 9) &&
          (local_area[W] == BALL)) return(KICK);
    if ((UN(near_object)(local_area, OPPONENT) < 9) &&
          (local_area[NW] == BALL)) return(KICK);

    /*
        Using my own team-mates as a reference, figure out where the bulk
        of the other players (both friend and foe) are at, and plan to
        kick AWAY from them (making ME the closest player to the ball after
        the kick).  If I'm near either the north or south edge, then
        this is an invalid indicator, and I know I should kick straight
        west.
    */
    for (i = 0; i < 4; ++i)
    {
        if ((y < 18) && (ply[i] < y)) ++kickSouth;
        if ((y > 4) && (ply[i] > y)) --kickSouth;
    }

    if (local_area[SW] == BALL)
    {
        if ((x > 60) ||          /* If near opponent's goal */
            (kickSouth > 0))     /* OR we should kick south */
           return KICK;          /* Then kick */

        return(S); /* Otherwise, move into position to kick straight west */
    }

    if (local_area[NW] == BALL)
    {
        if ((x > 60) ||          /* If near opponent's goal */
            (kickSouth < 0))     /* OR we should kick north */
           return KICK;          /* Then kick */

        return(N); /* Otherwise, move into position to kick straight west */
    }

    if (local_area[W] == BALL)
    {
        /*
           If we have a STRONG preference to kick the ball north or south,
           then move into position to do so.
        */
        if (kickSouth == 3) return N;
        else if (kickSouth == -3) return S;

        /* Otherwise, kick toward the opposing goal. */
        return KICK;
    }

    if (local_area[N] == BALL)
    {
        /*
           If an opponent is in position to kick the ball toward my goal,
           then kick it out of their grasp.  This defeats the driver for
           Diagonal Demons, among others.
        */
        if ((local_area[W] == OPPONENT) &&
            (local_area[NW] == OPPONENT)) return KICK;

        /*
           If near my own goal, get more desperate about kicking away the
           ball.
        */
        if ((x > 60) &&
            ((local_area[W] == OPPONENT) || (local_area[NW] == OPPONENT)))
            return KICK;

        /*
           If no opponents are about and I want to kick south, move into
           position to do so.
        */
        if ((kickSouth >= 0) && (local_area[NE] == EMPTY)) return NE;

        /* If I want to kick north, just move east so I can kick NW */
        return E;
    }

    if (local_area[NE] == BALL)
    {
        /*
           If there's an opponent who will get to the ball next turn,
           get between him and the ball.
        */
        if ((local_area[N] == EMPTY) && (local_area[NW] == OPPONENT)) return N;

        /* Otherwise, maneuver toward the east side of the ball. */
        return E;
    }

    if (local_area[E] == BALL)
    {
        /* Get into position to kick the ball, if possible */
        if ((kickSouth > 0) && (local_area[NE] == EMPTY)) return(NE);
        if ((kickSouth < 0) && (local_area[SE] == EMPTY)) return(SE);

        /* Otherwise, try and block the opponent (if any) */
        if (local_area[W] == OPPONENT) return W;

        /* Otherwise, just move out of the way. */
        if (local_area[N] == EMPTY) return N;
        return S;
    }

    if (local_area[SE] == BALL)
    {
        /*
           If there's an opponent who will get to the ball next turn,
           get between him and the ball.
        */
        if ((local_area[S] == EMPTY) && (local_area[SW] == OPPONENT)) return S;

        /* Otherwise, maneuver toward the east side of the ball. */
        return E;
    }

    if (local_area[S] == BALL)
    {
        /*
           If an opponent is in position to kick the ball toward my goal,
           then kick it out of their grasp.  This defeats the driver for
           Diagonal Demons, among others.
        */
        if ((local_area[W] == OPPONENT) &&
            (local_area[SW] == OPPONENT)) return KICK;

        /*
           If near my own goal, get more desperate about kicking away the
           ball.
        */
        if ((x > 60) &&
            ((local_area[W] == OPPONENT) || (local_area[SW] == OPPONENT)))
            return KICK;

        /*
           If no opponents are about and I want to kick north, move into
           position to do so.
        */
        if ((kickSouth <= 0) && (local_area[SE] == EMPTY)) return(SE);

        /* Otherwise, just get on the east side of the ball. */
        return E;
    }

    /*
     * If not near the ball, just go to it.
     */
    return(ball_direction);
}

/*-----------------------------------------------------

	northWing()

-----------------------------------------------------*/
int UN(northWing)(int local_area[9], int ball_direction, int x, int y)
{
    int ew = -1;
    int ns = -1;

    /* If near the ball, handle it like the leader would. */
    if (UN(near_object)(local_area, BALL) < 9)
    {
        return UN(lead)(local_area, ball_direction, x, y);
    }

    /* Determine which direction to move in to get into position */
    if ((local_area[N] == EMPTY) && (y > (ply[leader] - WINGSPAN))) ns = N;
    if ((local_area[S] == EMPTY) && (y < (ply[leader] - WINGSPAN))) ns = S;
    if ((x < plx[leader]) && (y == ply[leader])) ns = S;
    if ((local_area[W] == EMPTY) && (x > (plx[leader] + WINGSPAN))) ew = W;
    if ((local_area[E] == EMPTY) && (x < (plx[leader] + WINGSPAN))) ew = E;

    /* Combine preferred directions */
    if ((ew >= 0) && (ns >= 0))
    {
        if ((ew == E) && (ns == N)) return NE;
        if ((ew == E) && (ns == S)) return SE;
        if ((ew == W) && (ns == N)) return NW;
        if ((ew == W) && (ns == S)) return SW;
    }
    else if (ew >= 0) return ew;
    else if (ns >= 0) return ns;

    /* If in position, then move toward the ball. */
    return ball_direction;
}


/*-----------------------------------------------------

	southWing()

-----------------------------------------------------*/
int UN(southWing)(int local_area[9], int ball_direction, int x, int y)
{
    int ew = -1;
    int ns = -1;

    if (UN(near_object)(local_area, BALL) < 9)
    {
        return UN(lead)(local_area, ball_direction, x, y);
    }

    if ((local_area[N] == EMPTY) && (y > (ply[leader] + WINGSPAN))) ns = N;
    if ((local_area[S] == EMPTY) && (y < (ply[leader] + WINGSPAN))) ns = S;
    if ((x < plx[leader]) && (y == ply[leader])) ns = N;
    if ((local_area[E] == EMPTY) && (x < (plx[leader] + WINGSPAN))) ew = E;
    if ((local_area[W] == EMPTY) && (x > (plx[leader] + WINGSPAN))) ew = W;

    if ((ew >= 0) && (ns >= 0))
    {
        if ((ew == E) && (ns == N)) return NE;
        if ((ew == E) && (ns == S)) return SE;
        if ((ew == W) && (ns == N)) return NW;
        if ((ew == W) && (ns == S)) return SW;
    }
    else if (ew >= 0) return ew;
    else if (ns >= 0) return ns;

    return ball_direction;
}

/*-----------------------------------------------------

	REAR()

-----------------------------------------------------*/
int UN(rear)(int local_area[9], int ball_direction, int x, int y)
{
    return UN(lead)(local_area, ball_direction, x, y);
}

/*-----------------------------------------------------

	team_name()

	Every team must have a name.  Make sure it
	is exactly 20 characters.  Pad with spaces.

-----------------------------------------------------*/
char *UN(team_name)()
{
char	*s;

/*  "####################\0" <--- 20 characters */
s = "Dynamic Rollers  \0";
return(s);
}

/*-----------------------------------------------------

	initialize_game()

	This function is called only once per
	game, before play begins.  You can leave it
	empty, or put code here to initialize your
	variables, etc.

-----------------------------------------------------*/
void  UN(initialize_game)()
{
    WINGSPAN = 8;
    CONTROL_TIME = 13; /* 13 yields +38 index. 5 yields +26 index. */
}

/*-----------------------------------------------------

	initialize_point()

	This function is called once per point, just
	before play begins for that point.
	You can leave it empty, or put code here to initialize 
	your variables, etc.

-----------------------------------------------------*/
void  UN(initialize_point)()
{
    leader = -1;
    haveBall = 0;
    roles[0] = UN(lead);
    roles[1] = UN(northWing);
    roles[2] = UN(southWing);
    roles[3] = UN(rear);
}

/*-----------------------------------------------------

	lost_point()

	If your team loses a point, this function is
	called, otherwise, if you win the point, won_point()
	is called.  You can leave it empty, or put code here 
	for negative re-inforcement, etc.

-----------------------------------------------------*/
void  UN(lost_point)(void)
{
}

/*-----------------------------------------------------

	won_point()

	If your team wins a point, this function is
	called, otherwise, if you lose the point, lost_point()
	is called.  You can leave it empty, or put code here 
	for positive re-inforcement, etc.

-----------------------------------------------------*/
void  UN(won_point)(void)
{
}


/*-----------------------------------------------------

        game_over()

        This function is called once at the end of
        the game.  You can leave it empty, or put code 
        here to save things to a file, etc.

-----------------------------------------------------*/
void UN(game_over)()
{
}


