// 2009/01/29 Naoyuki Hirayama

#include "pathview_renderer.hpp"

const int BEZIER_DIVISION = 8;
const int DOT_DIVISION = 32;

/*============================================================================
 *
 * class PathviewRenderer 
 *
 * 
 *
 *==========================================================================*/
//<<<<<<<<<< PathviewRenderer

//****************************************************************
// render
void PathviewRenderer::render(
    LPDIRECT3DDEVICE9               device,
    const std::vector<Primitive>&   primitives,
    int                             step) {
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

    static vertex_type vertices0[32768*4];
    static vertex_type vertices1[32768*4];
    vertex_type* v0 = vertices0;
    vertex_type* v1 = vertices1;
    draw_primitives(primitives, step, v0, v1);

    int n0 =(v0 - vertices0)/ 2;
    int n1 =(v1 - vertices1)/ 3;

    device->SetFVF(vertex_type::format);
    device->DrawPrimitiveUP(
        D3DPT_TRIANGLELIST, n1, vertices1, sizeof(vertex_type));
    device->DrawPrimitiveUP(
        D3DPT_LINELIST, n0, vertices0, sizeof(vertex_type));
}

//----------------------------------------------------------------
// draw_primitives
void PathviewRenderer::draw_primitives(
    const std::vector<Primitive>&   primitives,
    int                             step,
    vertex_type*&                   v0,
    vertex_type*&                   v1) {

    D3DXVECTOR2 cursor(0, 0); 
    DWORD color = D3DCOLOR_ARGB(64, 0, 0, 0);

    for (int i = 0 ; i <int(primitives.size()) && i < step ; i++) {
        const Primitive& p = primitives[i];
        const float* a = p.operands;

        switch (p.opcode) {
            case Primitive::Color:
                color = D3DCOLOR_ARGB(
                    64,
                    int(a[0] * 255),
                    int(a[1] * 255),
                    int(a[2] * 255));
                break;
            case Primitive::MoveTo:
                cursor = vec2(a);
                break;
            case Primitive::LineTo:
                v0[0].p = vec4(cursor);
                v0[0].c = color;
                v0[1].p = vec4(vec2(a));
                v0[1].c = color;
                v0 += 2;
                cursor = vec2(a);
                break;
            case Primitive::Triangle:
                v1[0].p = vec4(cursor);
                v1[0].c = color;
                v1[1].p = vec4(vec2(a + 0));
                v1[1].c = color;
                v1[2].p = vec4(vec2(a + 2));
                v1[2].c = color;
                v1 += 3;
                break;
            case Primitive::Dot: {
#if 1
                float r = a[0];
                D3DXVECTOR2 c = cursor;
                for (int i = 0 ; i <DOT_DIVISION ; i++) {
                    float t0 =(1.0f / DOT_DIVISION)*(i);
                    float t1 =(1.0f / DOT_DIVISION)*(i + 1);

                    t0 *= D3DX_PI * 2.0f;
                    t1 *= D3DX_PI * 2.0f;

                    float x0 = cosf(t0)* r;
                    float y0 = sinf(t0)* r;
                    float x1 = cosf(t1)* r;
                    float y1 = sinf(t1)* r;

                    v0[0].p = vec4(c + D3DXVECTOR2(x0, y0));
                    v0[0].c = color;
                    v0[1].p = vec4(c + D3DXVECTOR2(x1, y1));
                    v0[1].c = color;
                    v0 += 2;
                }
#endif
                break;
            }
            default:
                break;
        }
    }
}

//>>>>>>>>>> PathviewRenderer

