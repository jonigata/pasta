// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  Partawn.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef PARTAWN_HPP_
#define PARTAWN_HPP_

#include "water.hpp"

class StandardPartawn : public TrivialPartawn {
public:
    StandardPartawn(TeamTag team_tag, const Vector& target, float speed)
        : TrivialPartawn(team_tag){
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
        if (0 < attack_count_) { attack_count_ = 0; return Vector(0,0); }
        Vector d = target_ - p;
        return d * (speed_ / D3DXVec2Length(&d));
    }

    void update(float elapsed) {
        life_ -= 0.1f * elapsed;
    }

    void preattack() {
        attack_count_++;
    }

    void attack(float elapsed, IPartawn* target) {
        target->suffer_damage(elapsed * 1.0f / attack_count_);
    }


private:
    Vector  target_;
    float   speed_;
    int     attack_count_;

};

#endif // PARTAWN_HPP_
