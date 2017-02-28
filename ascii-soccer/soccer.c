/*****************************************************************

	soccer.c

	A program to test and evaluate mutiagent soccer 
	strategies.  See docs/soccer.ps for a more complete 
	description.

	Copyright (C) 1995 Tucker Balch

	This program is free software; you can redistribute it and/or modify
    	it under the terms of the GNU General Public License as published by
    	the Free Software Foundation; either version 2 of the License, or
    	(at your option) any later version.

    	This program is distributed in the hope that it will be useful,
    	but WITHOUT ANY WARRANTY; without even the implied warranty of
    	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    	GNU General Public License for more details.

    	You should have received a copy of the GNU General Public License
    	along with this program; if not, write to the Free Software
    	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	Modification History:

	16 APR 95 1.0	Tucker Balch - original version.
	28 APR 95 1.1	Greg Eisenhauer - added some interactive
			options.

	19 OCT 95 1.3	Tucker Balch

	6  NOV 95 1.4	Tucker Balch - fixed some portability bugs.
	10 NOV 95 1.5	Tucker Balch - fixed more portability bugs.
	15 NOV 95 1.99	Tucker Balch - changed directory structure
				and linking. Added game_over() call
				for teams to save state.

	Bugs:
		East always gets to move first.

	Possible improvements:
		Full sensing of all other players.
		Sqrt of dx dy of other players.

*****************************************************************/

#include <curses.h>
#include <time.h>
#include "soccer.h"


WINDOW 	*game_win;

/*
 * Do not attempt to access these varibles from your
 * team function. That is a violation of the rules!
 */
int	ball_x, ball_y;
int	player_x[8], player_y[8];
int	field[MAX_X][MAX_Y];
int	display = 1; /* indicates whether or not to display stuff */
int	points = 7; /* How many points til a win ? */

extern	int	opterr;
extern	char	*optarg;


/******************************************************************

	report_score() 

******************************************************************/
void report_score(int west_score, int east_score)
{
char	score_line[100];

sprintf(score_line,
	"%s %d                              %s %d",
	WESTteam_name(), west_score, EASTteam_name(), east_score);
if (display) mvaddstr(MAX_Y, 0, score_line);
sprintf(score_line, "ASCII=Soccer=v2.0====(q)uit====(s)lower====(f)aster");
if (display) mvaddstr(0, 0, score_line);
if (display) wrefresh(game_win);
}


/******************************************************************

	replace_ball() 

	Put the ball in a random place.

******************************************************************/
void replace_ball()
{
if (display) mvaddch(ball_y, ball_x, ' ');
do
	{
	field[ball_x][ball_y] = EMPTY;
	ball_y = rand() % 20 + 1;
	}
while (field[ball_x][ball_y] != EMPTY);
field[ball_x][ball_y] = BALL;
if (display) mvaddch(ball_y, ball_x, 'O');
if (display) wrefresh(game_win);
}


/******************************************************************

	nudge_ball() 

******************************************************************/
void nudge_ball()
{
int i = 1;
int temp;

if (display) mvaddch(ball_y, ball_x, ' ');

do
	{
	temp = ball_y + ((rand()/256) % i) - (i/2) ;
	if (temp<1) temp = 1;
	if (temp>22) temp = 22;
	i++;
	if (i > 40) i = 40;
	}
while ((field[ball_x][temp] != EMPTY)&&(field[ball_x][temp] != GOAL));
field[ball_x][ball_y] = EMPTY;
ball_y = temp;
field[ball_x][ball_y] = BALL;
if (display) mvaddch(ball_y, ball_x, 'O');
}



/******************************************************************

	swap_heading() 

	Turn heading 180 degrees.

******************************************************************/
int swap_heading(int heading)
{
switch(heading)
	{
	case NW: return(SE); break;
	case N: return(S); break;
	case NE: return(SW); break;
	case E: return(W); break;
	case SE: return(NW); break;
	case S: return(N); break;
	case SW: return(NE); break;
	case W: return(E); break;
	case KICK: return(KICK);
	case DO_NOTHING: return(DO_NOTHING);
	default: return(DO_NOTHING); break;
	}
}


/******************************************************************

	init() 

	Initialize the playing field.

******************************************************************/
void init()
{
int	i, j;

/*--- Clear the screen ---*/
if (display) wclear(game_win);

/*--- Clear out the field map. ---*/
for(i=0; i<MAX_X; i++)
	{
	for(j=0; j<MAX_Y; j++)
		{
		field[i][j] = EMPTY;
		}
	}

/*--- Location of ball ---*/
ball_x = 38;
ball_y = 30;
replace_ball();

/*--- Locations of players ---*/
for(i = 0; i <=6; i+=2)
	{
	player_x[i] = 46;
	player_y[i] = (i/2+1)*6 - 4;
	field[player_x[i]][player_y[i]] = EAST_PLAYER;
	if (display) mvaddch(player_y[i], player_x[i], '<');

	player_x[i+1] = 30;
	player_y[i+1] = (i/2+1)*6 - 4;
	field[player_x[i+1]][player_y[i+1]] = WEST_PLAYER;
	if (display) mvaddch(player_y[i+1], player_x[i+1], '>');
	}

/*
 * Put boundary around the field.  
 * That will keep the players in bounds all the time.
 */
for(i=0; i<MAX_X; i++)
	{
	for(j=0; j<MAX_Y; j++)
		{
		if ((i==0)||(i==(MAX_X -1)))
			{
			field[i][j] = GOAL;
			if (display) mvaddch(j, i, '|');
			}
		if ((j==0)||(j==(MAX_Y-1)))
			{
			field[i][j] = BOUNDARY;
			if (display) mvaddch(j, i, '=');
			}
		}
	}

/*--- Refresh the screen ---*/
if (display) wrefresh(game_win);
}
		
    

/*****************************************************************

	main() 

	Sets up the environment and plays the game.

*****************************************************************/
int main(int argc, char *argv[])
{
char 	c;
int	i,j,k,cur_x,cur_y, backwards_cur_x, backwards_cur_y;
int	point_over = 0;
int	game_over = 0;
int	count, player_nearby = 0;
int	overall_count = 0;
int	local_field[9];
int	local_backwards_field[9];
int	local_ball_field[9];
int	ball_direction = N;
int	backwards_ball_direction = N;
int	player_move = N;
int	kick_direction = -1;
int	kick_steps = 0;
double	temp_angle = 0;
double	path = 0;
int	cur = 0;
int	east_score = 0, west_score = 0;
int	slow = 256;
int	last_ball_x = 0, last_ball_y = 0;

/*
 * Initialize stuff
 */
srand((unsigned int)time(NULL));
cur = 0;

/*
 * Scan the ole arglist
 */
opterr = 0;
while ((c = getopt(argc, argv, "dg:s:p:")) != -1)
	{
      	switch (c)
      		{
        	case 'd': display = 0; slow = 0; break;
		case 's':
            		{
               		int i;
               		if (sscanf(optarg, "%d", &i) == 0)
                  		fprintf(stderr, "Error reading seed! (%s)\n", optarg);
               		else
				srand(i);
            		}
            		break;
		case 'p':
            		{
               		int i;
               		if (sscanf(optarg, "%d", &i) == 0)
                  		fprintf(stderr, "Error reading points! (%s)\n", optarg);
               		else
				points = i;
			if (points <= 0 )
                  		fprintf(stderr, "Points must be > 0 (%s)\n", optarg);
            		}
            		break;

		default : break;
		}
	}

/*
 * Init screen
 */
if (display) initscr();
if (display) cbreak();
if (display) noecho();
if (display) game_win = subwin(stdscr, 25, COLS, 0, 0);
if (display) wmove(game_win,0,0);
if (display) wrefresh(game_win);

/*
 * Sleep to let user see what's up
 */
sleep(2);

/*
 * Call user initialization routines.
 */
EASTinitialize_game();
WESTinitialize_game();

/*
 * Do this until one team wins best of 7 points
 */
while (game_over != 1)
	{
	/*
	 * Set up for the point.
	 */
	init();
	if (display) report_score(west_score,east_score);
	point_over = 0;
	count = 0;
	overall_count = 0;
	kick_direction = -1;
	kick_steps = 0;
	sleep(1);

	/*
	 * Call user initialization functions.
	 */
	EASTinitialize_point();
	WESTinitialize_point();

	/*
	 * Play the point til one team wins.
	 */
	while (point_over != 1)
		{
		/*
		 * Player's posit
		 */
		cur_x = player_x[cur];
		cur_y = player_y[cur];

		/*
	 	* Note the local field around the player
	 	*/
		k = 0;
		for (j = 0; j < 3; j++)
			for (i = 0; i < 3; i++)
				{
				local_field[k++] = 
				field[cur_x+i-1][cur_y+j-1];
				}
		/*
	 	* Note the local field around the ball
	 	*/
		k = 0;
		for (j = 0; j < 3; j++)
			for (i = 0; i < 3; i++)
				{
				local_ball_field[k++] = 
				field[ball_x+i-1][ball_y+j-1];
				}
	
		/*
	 	* Figure out heading to ball
	 	*/
		if ((ball_x == cur_x) && (ball_y == cur_y))
			temp_angle = 0;
		else
			temp_angle = 
				atan2((ball_x - cur_x), 
				(ball_y - cur_y));
		temp_angle += PI;
		temp_angle = 360.0*temp_angle/(2.0*PI);
		ball_direction = N;
		if (temp_angle > 22.5+0*45)
			ball_direction = NW;
		if (temp_angle > 22.5+1*45)
			ball_direction = W;
		if (temp_angle > 22.5+2*45)
			ball_direction = SW;
		if (temp_angle > 22.5+3*45)
			ball_direction = S;
		if (temp_angle > 22.5+4*45)
			ball_direction = SE;
		if (temp_angle > 22.5+5*45)
			ball_direction = E;
		if (temp_angle > 22.5+6*45)
			ball_direction = NE;
		if (temp_angle > 22.5+7*45)
			ball_direction = N;
	
		/*
		 * Construct backwards sensing for western players.
		 */
		for (i = 0; i <=8; i++)
			local_backwards_field[i]=local_field[8-i];
		backwards_ball_direction = swap_heading(ball_direction);
		backwards_cur_x = (MAX_X - 1) - cur_x;
		backwards_cur_y = (MAX_Y - 1) - cur_y;

		/*
	 	 * Find out which way to go. The variable cur
		 * indicates which player we are moving for now.
		 * East players are 0 2 4 6.  The local_field and
		 * ball_direction are reversed for west players
		 * so they can think they are going east!  Then
		 * their output is reversed again.
	 	 */
		switch(cur)
			{
			case 0: player_move = 
				EASTplayer1(local_field, ball_direction,
					cur_x, cur_y);
 				break;
			case 1: player_move = 
				swap_heading(
				WESTplayer1(local_backwards_field, 
					backwards_ball_direction,
					backwards_cur_x, backwards_cur_y));
 				break;
			case 2: player_move = 
				EASTplayer2(local_field, ball_direction,
					cur_x, cur_y);
 				break;
			case 3: player_move = 
				swap_heading(
				WESTplayer2(local_backwards_field, 
					backwards_ball_direction,
					backwards_cur_x, backwards_cur_y));
 				break;
			case 4: player_move = 
				EASTplayer3(local_field, ball_direction,
					cur_x, cur_y);
 				break;
			case 5: player_move = 
				swap_heading(
				WESTplayer3(local_backwards_field, 
				backwards_ball_direction,
					backwards_cur_x, backwards_cur_y));
 				break;
			case 6: player_move = 
				EASTplayer4(local_field, ball_direction,
					cur_x, cur_y);
 				break;
			case 7: player_move = 
				swap_heading(
				WESTplayer4(local_backwards_field, 
				backwards_ball_direction,
					backwards_cur_x, backwards_cur_y));
 				break;
			}

		/*
		 * Check for move = PLAYER 
		 */
		if (player_move == PLAYER)
			{
			player_move = DO_NOTHING;
			}

		/*
		 * Check for KICK.  If the player wants to kick
		 * set up the kick, then set player_move to be
		 * heading in the direction of the ball.
		 */
		if (player_move == KICK)
			{
			for(i = 0; i<=8; i++)
				{
				/*--- Where is the ball ?---*/
				if (local_field[i] == BALL)
					{
					kick_direction = i;
					kick_steps = KICK_DIST;
					player_move = i;
					break;
					}
				}
			if (player_move == KICK)
				{
				/*--- Ball was NOT nearby ---*/
				player_move = N;
				}
			}

		/*
		 * Make sure heading is legal.
		 */
		if ((player_move < 0) || (player_move > 8))
			{
			player_move = DO_NOTHING;
			}

		/*
	 	* Move the player in the world
	 	*/
		if (display) mvaddch(cur_y, cur_x, ' '); /* erase old spot */
		field[cur_x][cur_y] = EMPTY;

		/*
		 * If the cell we are going to is empty, or
		 * the ball is there and the next cell after that is
		 * empty, then . . .
		 */
		if (player_move != DO_NOTHING &&
             ((local_field[player_move] == EMPTY) ||
			  ((local_field[player_move] == BALL)&&
				((local_ball_field[player_move] == EMPTY)||
				(local_ball_field[player_move] == GOAL)))))
			{
			switch(player_move)
				{
				case N: player_y[cur]--; path++; break;
				case S: player_y[cur]++; path++; break;
				case E: player_x[cur]++; path++; break;
				case W: player_x[cur]--; path++; break;
				case NE: player_y[cur]--; player_x[cur]++; 
					path+=SQR2; break;
				case NW: player_y[cur]--; player_x[cur]--; 
					path+=SQR2; break;
				case SE: player_y[cur]++; player_x[cur]++; 
					path+=SQR2; break;
				case SW: player_y[cur]++; player_x[cur]--; 
					path+=SQR2; break;
				}
			}

		/*
		 * Mark the field with the player's new position.
		 */
		if ((cur % 2) == 0)
			field[player_x[cur]][player_y[cur]] = EAST_PLAYER;
		else
			field[player_x[cur]][player_y[cur]] = WEST_PLAYER;

		/*
		 * Now move the ball.
		 */
		if ((local_field[player_move] == BALL)&&
			((local_ball_field[player_move] == EMPTY)||
			(local_ball_field[player_move] == GOAL)))
			{
			field[ball_x][ball_y] = EMPTY;
			if (display) mvaddch(ball_y, ball_x, ' '); /* new position */
			switch(player_move)
				{
				case N: ball_y--; break;
				case S: ball_y++; break;
				case E: ball_x++; break;
				case W: ball_x--; break;
				case NE: ball_y--; ball_x++; break;
				case NW: ball_y--; ball_x--; break;
				case SE: ball_y++; ball_x++; break;
				case SW: ball_y++; ball_x--; break;
				}
			}
		/*
		 * Place the ball on the field.
		 */
		field[ball_x][ball_y] = BALL;

		/*
	 	 * Now handle the case of a KICK
	 	 */
		if (kick_steps > 0)
			{
			/*--- Revise the local ball field ---*/
			k = 0;
			for (j = 0; j < 3; j++)
				for (i = 0; i < 3; i++)
					{
					local_ball_field[k++] = 
					field[ball_x+i-1][ball_y+j-1];
					}

			/*--- Erase old position ---*/
			field[ball_x][ball_y] = EMPTY;
			if (display) mvaddch(ball_y, ball_x, ' ');
			
			/*--- Propel the ball if the space is empty ---*/
			if ((local_ball_field[kick_direction] == EMPTY) ||
				(local_ball_field[kick_direction] == GOAL))
				switch(kick_direction)
					{
					case N: ball_y--; break;
					case S: ball_y++; break;
					case E: ball_x++; break;
					case W: ball_x--; break;
					case NE: ball_y--; ball_x++; break;
					case NW: ball_y--; ball_x--; break;
					case SE: ball_y++; ball_x++; break;
					case SW: ball_y++; ball_x--; break;
					default: break;
					}
			else
				{
				kick_direction = -1;
				kick_steps = 0;
				}
			kick_steps--;
			field[ball_x][ball_y] = BALL;
			}
			
	
		/*
		 * Mark the new locations of the ball and
		 * the player on the field.
		 */
		if (display) mvaddch(ball_y, ball_x, 'O');
		if ((cur % 2) == 0)
			{
			if (display) mvaddch(player_y[cur], player_x[cur], '<');
			}
		else
			{
			if (display) mvaddch(player_y[cur], player_x[cur], '>');
			}
		if (display) wrefresh(game_win);

		/*
		 * Check for a score.
		 */
		if (ball_x <= 0)
			{
			east_score++;
			point_over = 1;
			if (!display) 
				{
				printf("%s vs %s: %d to %d \n",WESTteam_name(),EASTteam_name(),
				west_score,east_score);
				fflush(stdout);
				}
			EASTwon_point(); /* Advise the teams of the point */
			WESTlost_point();
			}
		if (ball_x >= (MAX_X-1))
			{
			west_score++;
			point_over = 1;
			if (!display) 
				{
				printf("%s vs %s: %d to %d \n",WESTteam_name(),EASTteam_name(),
				west_score,east_score);
				fflush(stdout);
				}
			WESTwon_point(); /* Advise the teams of the point */
			EASTlost_point();
			}

		/*
		 * Check for a stalemate.
		 */
		player_nearby = 0;
		for (i = 0; i<=8; i++)
			{
			if ((local_ball_field[i] == WEST_PLAYER) ||
				(local_ball_field[i] == EAST_PLAYER))
				player_nearby = 1;	
			}
		if (player_nearby && 
			((last_ball_x == ball_x)&&(last_ball_y == ball_y)))
			count++;
		if (count > TIME_LIMIT)
			{
			nudge_ball();
			count = 0;
			}
		if ((last_ball_x != ball_x)||(last_ball_y != ball_y))
			count = 0;
		last_ball_x = ball_x;
		last_ball_y = ball_y;

		/*
		 * Check for user input.
		 */
		if (display) 
			{
		      	fd_set inset;
		      	struct timeval timeout;
		      	FD_ZERO(&inset);
		      	FD_SET(0, &inset);
		      	timeout.tv_sec = timeout.tv_usec = 0;
		      	if (select(FD_SETSIZE, 
				&inset, NULL, NULL, &timeout) > 0) 
				{
			    	int input = getch();
			    	if (input == 'q') point_over = game_over = 1;
			    	if (input == 's') slow*=2;
			    	if (input == 'f') slow/=2;
			    	if (slow < 1) slow=1;
				}
			if (slow>1) 
				{
		  		timeout.tv_usec = (slow-1)*20;
		  		select(32, NULL, NULL, NULL, &timeout);
				}
		  	}

		/*
		 * Go on to the next player.
		 */
		cur++;
		if (cur == 8) 
		      	cur = 0;

		/*
		 * Check for big timeout.
		 */
		overall_count++;
		if (overall_count >= TIME_OUT)
			{
			printf("TIME_OUT\n");
			/* erase old spot */
			if (display) mvaddch(ball_y, ball_x, ' ');
			field[ball_x][ball_y] = EMPTY;
			ball_x = 38;
			ball_y = 30;
			replace_ball();
			overall_count = 0;
			EASTlost_point(); /* punish teams */
			WESTlost_point();
			if (display) wrefresh(game_win);
			}
		}

	/*
	 * Check for first to 7 wins.
	 */ 
	if ((west_score == points) || (east_score == points))
		game_over = 1;
	}
EASTgame_over();
WESTgame_over();
if (display) report_score(west_score, east_score);
if (display) endwin();
printf("\n");
printf("%s vs %s: %d to %d \n",WESTteam_name(),EASTteam_name(),
	west_score,east_score);
if (west_score < east_score)
	printf("%s won\n",EASTteam_name());
else
	printf("%s won\n",WESTteam_name());
}
