// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  basecamp.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef BASECAMP_HPP_
#define BASECAMP_HPP_

#include "castle.hpp"

class Basecamp : public ImmovablePartawn {
public:
    Basecamp(TeamTag team_tag, Castle& castle, const Vector& origin)
        : ImmovablePartawn(team_tag, origin), castle_(castle) {
    }

    void update(float elapsed) {}

private:
    Castle& castle_;

};
#endif // BASECAMP_HPP_
