BEHAVIOR=example/behavior.h \
         example/options.h \
         example/options/defender.h \
         example/options/dribble.h \
         example/options/get_behind_ball.h \
         example/options/go_dir.h \
         example/options/go_to.h \
         example/options/midfielder.h \
         example/options/pass.h \
         example/options/play_soccer.h \
         example/options/set_action.h \
         example/options/striker.h \
         include/Cabsl.h \
         include/ActivationGraph.h

soccer: soccer.o rollers.o behavior.o cabsl.o
	gcc -w soccer.o rollers.o behavior.o cabsl.o -o soccer -lncurses -lm -lstdc++

soccer.o: ascii-soccer/soccer.c ascii-soccer/soccer.h ascii-soccer/players.h
	gcc -w -c ascii-soccer/soccer.c

rollers.o: ascii-soccer/teams/rollers/main.c ascii-soccer/soccer.h ascii-soccer/players.h
	gcc -w -Iascii-soccer -DWEST_TEAM -c ascii-soccer/teams/rollers/main.c -o rollers.o

behavior.o: example/behavior.cpp ascii-soccer/soccer.h $(BEHAVIOR)
	g++ -w -std=c++11 -Iascii-soccer -Iinclude -c example/behavior.cpp

cabsl.o: example/main.cpp ascii-soccer/soccer.h ascii-soccer/players.h $(BEHAVIOR)
	g++ -w -std=c++11 -Iascii-soccer -Iinclude -DEAST_TEAM -c example/main.cpp -o cabsl.o

graphs:
	bin/createGraphs example/options.h

clean: 
	rm -f soccer *.o *.pdf
