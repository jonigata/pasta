// 2016/03/21 Naoyuki Hirayama

/*!
	@file	  ai.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef AI_HPP_
#define AI_HPP_

#include <cstdlib> //rand

class Ai {
public:
    Ai(Board& board) : board_(board) {}

    void think(float elapsed) {
        if (rand() % 10 < 1) {
            Vector v(rand() % 512, rand() % 512);
            auto team = board_.team(TeamTag::Beta);
            if (team->in_teritory(v)) {
                board_.settle_station(TeamTag::Beta, v, Vector(64, 448));
            }
        }
    }

private:
    Board& board_;

};

#endif // AI_HPP_
