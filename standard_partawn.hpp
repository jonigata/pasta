// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  Partawn.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef PARTAWN_HPP_
#define PARTAWN_HPP_

#include "water.hpp"

class StandardPartawn : public IPartawn {
public:
    StandardPartawn(const Vector& target, float speed) {
        target_ = target;
        speed_ = speed;
    }

    Vector constraint_velocity(const Vector& av) {
        Vector v = av;

        float maxSpeed = 100.0f;
        float l = D3DXVec2LengthSq(&v);
        if (maxSpeed * maxSpeed < l) {
            v *= (1.0f / sqrt(l)); 
        }

        return v;
    }

    Vector move(const Vector& p) {
        Vector d = target_ - p;
        return d * (speed_ / D3DXVec2Length(&d));
    }

    void update(float elpased) {
    }

private:
    Vector  target_;
    float   speed_;

};

#endif // PARTAWN_HPP_
