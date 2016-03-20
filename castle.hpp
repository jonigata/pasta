// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  castle.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef CASTLE_HPP_
#define CASTLE_HPP_

#include <deque>

class Castle {
public:
    struct EmitEntry {
        Vector origin;
        Vector target;
    };

    Castle() {
    }

    void emit(const EmitEntry& e) {
        emit_queue_.push_back(e);
    }

    bool fetch(EmitEntry& e) {
        if (emit_queue_.empty()) {
            return false;
        }
        e = emit_queue_.front();
        emit_queue_.pop_front();
        return true;
    }

private:
    std::deque<EmitEntry> emit_queue_;

};

#endif // CASTLE_HPP_
