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
        if (in_teritory(*board_.basecamp(), v)) {
            board_.settle_station(v, Vector(256, -256));
        } else {
            for (auto& s: board_.stations()) {
                if (in_teritory(*s, v)) {
                    board_.settle_station(v, Vector(256, -256));
                    return;
                }
            }
        }
    }


    void think() {
    }

private:
    bool in_teritory(TrivialPartawn& p, const Vector& v) {
        Vector d = p.location() - v;
        return D3DXVec2Length(&d) < 50.0f;
    }

    Board& board_;

};

#endif // PLAYER_HPP_
