// 2008/12/25 Naoyuki Hirayama

#include "water.hpp"
#include "zw/dprintf.hpp"
#include "performance_counter.hpp"

const float DISPLAY_MAG			   = 1.0f;
const float DOT_SIZE			   = 9.0f;

namespace {

class Renderer {
public:
    typedef zw::fvf::vertex<D3DFVF_XYZRHW|D3DFVF_DIFFUSE> vertex_type;

public:
    Renderer(vertex_type* v) : vertices(v) {
    }
    vertex_type* vertices;

    D3DXVECTOR2 offmag(const D3DXVECTOR2& v) {
        return v *(DISPLAY_MAG);
    }

    void operator()(
        int     id,
        const D3DXVECTOR2& pos,
        float    mass,
        float    density_plain,
        float    density_balance_corrected,
        float    density_repulsive_corrected,
        float    boundariness,
        const WaterTraits::load_type& load) {

        DWORD c = D3DCOLOR_ARGB(int(load->life() * 255), 16, 0, 255);

        Vector sx(DOT_SIZE, 0);
        Vector sy(0, DOT_SIZE);
        Vector sxy(DOT_SIZE, DOT_SIZE);
        Vector pos2 = pos - sxy * 0.5f;

        vertices[0].p = v4(offmag(pos2));
        vertices[1].p = v4(offmag(pos2)+ sx);
        vertices[2].p = v4(offmag(pos2)+ sy);
        vertices[3].p = v4(offmag(pos2)+ sy);
        vertices[4].p = v4(offmag(pos2)+ sx);
        vertices[5].p = v4(offmag(pos2)+ sxy);

        for (int j = 0 ; j <6 ; j++) {
            vertices[j].c = c;
        }

        vertices += 6;
    }

    void render_frame() {
    }

    D3DXVECTOR4 v4(const Vector& v) {
        return D3DXVECTOR4(v.x, v.y, 0, 1);
    }
};

struct RendererWrapper {
    RendererWrapper(Renderer& ar) : r(ar) {}
    Renderer& r;

    void operator()(
        int     id,
        const Vector& pos,
        float    mass,
        float    density_plain,
        float    density_balance_corrected,
        float    density_repulsive_corrected,
        float    boundariness,
        const WaterTraits::load_type& load) {
        r(
            id,
            pos,
            mass,
            density_plain,
            density_balance_corrected,
            density_repulsive_corrected,
            boundariness,
            load);
    }
};

}


/*============================================================================
 *
 * class Water 
 *
 * 
 *
 *==========================================================================*/
//<<<<<<<<<< Water

//****************************************************************
// constructor
Water::Water() {
    constraint_ = NULL;

    sph_.initialize(
        SEARCH_RADIUS,
        VISCOSITY,
        DUMPING,
        Vector(0, GRAVITY),
        IDEAL_DENSITY,
        PRESSURE_BALANCE_COEFFICIENT,
        PRESSURE_REPULSIVE_COEFFICIENT);
}

//****************************************************************
// render
void Water::add(const Vector& v, float mass, IPartawn* partawn) {
    sph_.add_particle(v, mass, partawn);
}

//****************************************************************
// render
void Water::render(LPDIRECT3DDEVICE9 device) {
    typedef Renderer::vertex_type vertex_type;

    static vertex_type vertices[32768];

    Renderer r(vertices);
    sph_.foreach(RendererWrapper(r));

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
        (r.vertices - vertices)/ 3,
        vertices,
        sizeof(vertex_type));
}

//****************************************************************
// update
void Water::update() {
    //PerformanceCounter pc(true);
    sph_.update(0.01f);
    //pc.print("update");

    if (constraint_) {
        sph_.constraint(
            [this](const Vector& v){return constraint_->apply(v);});
        //pc.print("constraint");
    }

    sph_.discard(
        [](IPartawn* load) {
            return load->life() < 0.0f;
        });
}

//****************************************************************
// set_viscosity
void Water::set_viscosity(float v) {
    sph_.set_viscosity(v);
}

//****************************************************************
// get_viscosity
float Water::get_viscosity() {
    return sph_.get_viscosity();
}

//****************************************************************
// set_dumping
void Water::set_dumping(float d) {
    if (1.0f <d) { d = 1.0f; }
    sph_.set_dumping(d);
}

//****************************************************************
// get_dumping
float Water::get_dumping() {
    return sph_.get_dumping();
}

//****************************************************************
// set_ideal_density
void Water::set_ideal_density(float x) {
    sph_.set_ideal_density(x);
}

//****************************************************************
// get_ideal_density
float Water::get_ideal_density() {
    return sph_.get_ideal_density();
}

//****************************************************************
// set_constraint
void Water::set_constraint(IConstraint* constraint) {
    constraint_ = constraint;
}

//>>>>>>>>>> Water

