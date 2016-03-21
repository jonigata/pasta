// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  station.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef STATION_HPP_
#define STATION_HPP_

#include "castle.hpp"

class Station : public ImmovablePartawn {
public:
    Station(
        Castle& castle, 
        const Vector& origin, 
        const Vector& target, 
        float speed)
        : castle_(castle), ImmovablePartawn(origin) {
        target_ = target;
        speed_ = speed;
        timer_ = 1.0f;
    }

    void update(float elapsed) {
        timer_ -= elapsed;
        if (timer_ <= 0) {
            timer_ = 1.0f;
            castle_.emit(Castle::EmitEntry { location_, target_ });
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
