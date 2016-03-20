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
        board_.settle_station(v, Vector(256, -256));
    }


    void think() {
    }

private:
    Board& board_;

};

#endif // PLAYER_HPP_
