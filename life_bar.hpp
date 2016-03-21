// 2016/03/21 Naoyuki Hirayama

/*!
	@file	  life_bar.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef LIFE_BAR_HPP_
#define LIFE_BAR_HPP_

#include "zw/d3dfvf.hpp"

class LifeBar {
private:
    typedef zw::fvf::vertex<D3DFVF_XYZRHW|D3DFVF_DIFFUSE> vertex_type;

public:
    LifeBar() { value_ = 0.5f; }

    void set_value(float x) { value_ = x; }

    void render(LPDIRECT3DDEVICE9 device) {
        static vertex_type vertices[32768];

        Vector p0(528, 0);
        Vector p1(544, 512);
        Vector p2(528, p1.y + (p0.y - p1.y) * value_);
        draw_rectangle(vertices, p0, p1, D3DCOLOR_ARGB(255, 192, 192, 192));
        draw_rectangle(vertices+6, p2, p1, D3DCOLOR_ARGB(255, 128, 0, 255));
                       
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        device->SetFVF(vertex_type::format);

        device->DrawPrimitiveUP(
            D3DPT_TRIANGLELIST,
            4,
            vertices,
            sizeof(vertex_type));
    }

    void draw_rectangle(
        vertex_type* vertices,
        const Vector& pos1,
        const Vector& pos2,
        DWORD c) {

        Vector sx(pos2.x - pos1.x, 0);
        Vector sy(0, pos2.y - pos1.y);
        Vector sxy = sx + sy;

        vertices[0].p = v4(pos1);
        vertices[1].p = v4(pos1 + sx);
        vertices[2].p = v4(pos1 + sy);
        vertices[3].p = v4(pos1 + sy);
        vertices[4].p = v4(pos1 + sx);
        vertices[5].p = v4(pos1 + sxy);

        for (int j = 0 ; j <6 ; j++) {
            vertices[j].c = c;
        }
    }

private:
    D3DXVECTOR4 v4(const Vector& v) {
        return D3DXVECTOR4(v.x, v.y, 0, 1);
    }

    float value_;
};

#endif // LIFE_BAR_HPP_
