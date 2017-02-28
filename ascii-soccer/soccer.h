#ifndef SOCCER_H
	#define SOCCER_H

  #include <math.h>
  #include <sys/types.h>
  #include <sys/time.h>
  #include <memory.h>
  #include <unistd.h>
  #include <stdlib.h>


	#define MAX_X	80
	#define MAX_Y	23
	#define TIME_LIMIT	20
	#define TIME_OUT	3000
	#define KICK_DIST	10
	
	#ifndef PI
	#define PI	3.1415927
	#endif
	
	#define SQR2	1.4142136
	
	#define EMPTY	0
	#define GOAL	1
	#define BALL	2
	#define BOUNDARY	3
	#define	WEST_PLAYER	6
	#define EAST_PLAYER	7
	#define BIGGEST_SIT	7
	
	#define NW	0
	#define N	1
	#define NE	2
	#define W	3
	#define PLAYER	4
	#define E	5
	#define SW	6
	#define S	7
	#define SE	8
	
	#define	KICK	9
	#define	DO_NOTHING	10
	#define BIGGEST_ACTION	10

	void EASTinitialize_game(void);
	void EASTgame_over(void);
	void EASTinitialize_point(void);
	void EASTwon_point(void);
	void EASTlost_point(void);
	int EASTplayer1(int *, int, int, int);
	int EASTplayer2(int *, int, int, int);
	int EASTplayer3(int *, int, int, int);
	int EASTplayer4(int *, int, int, int);

	void WESTinitialize_game(void);
	void WESTgame_over(void);
	void WESTinitialize_point(void);
	void WESTwon_point(void);
	void WESTlost_point(void);
	int WESTplayer1(int *, int, int, int);
	int WESTplayer2(int *, int, int, int);
	int WESTplayer3(int *, int, int, int);
	int WESTplayer4(int *, int, int, int);

	char *EASTteam_name();
	char *WESTteam_name();

#endif
