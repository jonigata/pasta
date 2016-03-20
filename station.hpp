// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  station.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef STATION_HPP_
#define STATION_HPP_

#include "castle.hpp"

class Station : public IPartawn {
public:
    Station(
        Castle& castle, 
        const Vector& origin, 
        const Vector& target, 
        float speed)
        : castle_(castle) {
        origin_ = origin;
        target_ = target;
        speed_ = speed;
        timer_ = 1.0f;
    }

    Vector constraint_velocity(const Vector& av) {
        return Vector(0, 0);
    }

    Vector move(const Vector& p) {
        return Vector(0, 0);
    }

    void update(float elapsed) {
        timer_ -= elapsed;
        if (timer_ <= 0) {
            timer_ = 1.0f;
            castle_.emit(Castle::EmitEntry { origin_, target_ });
        }
    }

private:
    Castle& castle_;
    Vector  origin_;
    Vector  target_;
    float   speed_;
    float   timer_;

};

#endif // STATION_HPP_
