// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  board_renderer.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef BOARD_RENDERER_HPP_
#define BOARD_RENDERER_HPP_

#include "pathview.hpp"
#include "pathview_renderer.hpp"

class BoardRenderer {
public:
    BoardRenderer(Board& board) : board_(board) {}

    void setup() {
    }

    void render(LPDIRECT3DDEVICE9 device) {
        if (!board_.ready()) { return; }

        terrain_renderer_.render(
            device,
            board_.terrain_primitives(), 
            board_.terrain_primitives().size());
        board_.water().render(device);
    }

private:

private:
    Board&              board_;
    PathviewRenderer    terrain_renderer_;
    
};

#endif // BOARD_RENDERER_HPP_
