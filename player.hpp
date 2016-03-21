// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  player.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef PLAYER_HPP_
#define PLAYER_HPP_

class Player {
public:
    Player(Board& board) : board_(board) {}

    void tap(const Vector& v) {
        if (board_.in_teritory(TeamTag::Alpha, v)) {
            board_.settle_station(TeamTag::Alpha, v, Vector(448, 64));
        }
    }

private:

    Board& board_;

};

#endif // PLAYER_HPP_
