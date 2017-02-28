#include "soccer.h"
#ifndef PLAYERS_H
	#define PLAYERS_H

	#ifdef EAST_TEAM
		#define OPPONENT WEST_PLAYER
		#define TEAMMATE EAST_PLAYER
		#define UN(original_name) EAST ## original_name
	#endif

	#ifdef WEST_TEAM
		#define OPPONENT EAST_PLAYER
		#define TEAMMATE WEST_PLAYER
		#define UN(original_name) WEST ## original_name
	#endif
#define unique_name(a) UN(a)
#endif
