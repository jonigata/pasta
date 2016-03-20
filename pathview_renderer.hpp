// 2009/01/29 Naoyuki Hirayama

/*!
	@file	  pathview_renderer.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef PATHVIEW_RENDERER_HPP_
#define PATHVIEW_RENDERER_HPP_

#include "zw/d3dfvf.hpp"
#include "pathview.hpp"

class PathviewRenderer {
private:
    typedef zw::fvf::vertex< (D3DFVF_XYZRHW|D3DFVF_DIFFUSE) > vertex_type;

public:
    PathviewRenderer() {}
    ~PathviewRenderer() {}

    void render(
        LPDIRECT3DDEVICE9    device,
        const std::vector<Primitive>& primitives,
        int        step);

private:
    void draw_primitives(
        const std::vector<Primitive>& primitives,
        int        step,
        vertex_type*&     v0,
        vertex_type*&     v1);

    D3DXVECTOR2 vec2(const float* a) {
        return D3DXVECTOR2(a[0], a[1]);
    }

    D3DXVECTOR4 vec4(const D3DXVECTOR2& v) {
        return D3DXVECTOR4(v.x, v.y, 0, 1);
    }

    enum {
        ZOOM_DIVISION = 1000,
    };

private:
    D3DXVECTOR2    offset_;
    D3DXVECTOR2    tmp_offset_;
    int      zoom_;

};

#endif // PATHVIEW_RENDERER_HPP_
