// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  station.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef STATION_HPP_
#define STATION_HPP_

#include "castle.hpp"
#include "team_tag.hpp"

class Station : public ImmovablePartawn {
public:
    Station(
        TeamTag team_tag,
        Castle& castle, 
        const Vector& origin, 
        const Vector& target, 
        float speed)
        : ImmovablePartawn(team_tag, origin), castle_(castle)  {
        target_ = target;
        speed_ = speed;
        timer_ = 1.0f;
    }

    void update(float elapsed) {
        timer_ -= elapsed;
        if (timer_ <= 0) {
            timer_ = 1.0f;
            castle_.emit(Castle::EmitEntry { team_tag_, location_, target_ });
        }
        life_ -= 0.01f * elapsed;
    }

private:
    Castle& castle_;
    Vector  target_;
    float   speed_;
    float   timer_;

};

#endif // STATION_HPP_
