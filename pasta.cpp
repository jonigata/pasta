// All Rights Reserved.
// $Id: sph2d.cpp 244 2007-05-02 08:57:27Z naoyuki $

#include <boost/random.hpp>

#include "zw/window_manager.hpp"
#include "zw/basic_window.hpp"
#include "zw/d3d.hpp"
#include "zw/game_timer.hpp"

#include "zw/user/dashboard.hpp"
#include "zw/user/empty_user.hpp"
#include "zw/user/window_user.hpp"
#include "zw/user/dashboard_user.hpp"
#include "zw/user/d3d_user.hpp"
#include "zw/user/d3d_draw.hpp"

#include "board.hpp"
#include "board_renderer.hpp"
#include "gci.hpp"
#include "player.hpp"

////////////////////////////////////////////////////////////////
// main window
typedef zw::win::basic_window<
    zw::win::event::create,
    zw::win::event::size,
    zw::win::event::paint,
    zw::win::event::mouse,
    zw::win::event::keychar
    > window_type;

class application
    : public window_user<application, window_type>,
      public d3d_user<application>,
      public dashboard_user<application> {

    typedef window_user<application, window_type> window_user_type;

public:
    application() : board_renderer_(board_), player_(board_) {
        auto_update_ = false;
        click_ = false;

        dragging_ = false;
    }
    ~application() {}

public:
    void accept(zw::win::event::create& m) {
        window_user_type::accept(m);
        board_.setup();
        board_renderer_.setup();
        font_vault.setup(d3d.device());
    }

    void accept(zw::win::event::size& m) {
        window_user_type::accept(m);
    }

    void accept(zw::win::event::paint& m) {
        window_user_type::accept(m);
    }

    void accept(zw::win::event::mouse& m) {
        window_user_type::accept(m);

        if (m.lbutton) {
            if (!dragging_) {
                dragging_ = true;
                player_.tap(Vector(m.position.dx(), m.position.dy()));
            }
        } else {
            dragging_ = false;
        }
    }

    void accept(zw::win::event::keychar& m) {
        if (m.code == VK_ESCAPE) {
            auto_update_ = !auto_update_;
        }
    }

    void on_timer(int elapsed0, float elapsed1) {
        d3d_user::on_timer(elapsed0, elapsed1);
        if (auto_update_ || click_) {
            board_.update();
            //window.invalidate();
            click_ = false;
        }
    }

    void render() {
        d3d.check_device_lost(*this);
        d3d.render(D3DCOLOR_ARGB(255, 255, 255, 255), *this);
        // d3d calls on_render here
    }

    void on_render(LPDIRECT3DDEVICE9 device) {
        board_renderer_.render(device);
    }

public:
    zw::font_vault    font_vault;
    zw::texture_vault texture_vault;

private:
    bool     auto_update_;
    bool     click_;

    bool     dragging_;
    zw::win::offset_type dragstart_;

    Board board_;
    BoardRenderer board_renderer_;
    Player player_;

};

extern "C"
int PASCAL WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow) 
{
    application a;
    a.run();
    return 0;
}

