// 2008/10/27 Naoyuki Hirayama

/*!
	@file	  water.hpp
	@brief	  <äTóv>

	<ê‡ñæ>
*/

#ifndef WATER_HPP_
#define WATER_HPP_

#include "zw/d3dfvf.hpp"
#include <boost/ref.hpp>
#include "sph.hpp"

const int HCOUNT				   = 10;
const int VCOUNT				   = 10;

const float INITIAL_DISTANCE	   = 10.0f;	// 1cm(10mm)ä‘äuÇÃäiéqÇçÏÇÈ
const float DT                     = 0.01f;	// 100ÉtÉåÅ[ÉÄ/s
const float SEARCH_RADIUS          = 50.0f;	// 5cm(50mm)ÇÃåüçıîÕàÕ
const float VISCOSITY              = 1.0f;
const float DUMPING                = 0.99f;
const float GRAVITY                = 0.0f;	// mm/(s*s)
const float MASS                   = 1.0f;
const float IDEAL_DENSITY          = 7.0f; // 10.0f;
const float PRESSURE_BALANCE_COEFFICIENT   = 0.0f;
const float PRESSURE_REPULSIVE_COEFFICIENT = 5.0f;

class IConstraint {
public:
    virtual ~IConstraint() {}

    virtual D3DXVECTOR2 apply(const D3DXVECTOR2&) = 0;
};


class Water {
public:
    Water();
    ~Water() {}

    void render(
        LPDIRECT3DDEVICE9 device,
        const D3DXVECTOR2& offset,
        float zoom);

    void click(D3DXVECTOR2& p) {}
    void update();

    void  set_viscosity(float);
    float  get_viscosity();
    void  set_dumping(float);
    float  get_dumping();
    void  set_ideal_density(float);
    float  get_ideal_density();

    void  set_constraint(IConstraint* constraint);

private:
    sph::sph<SPH_Traits_D3DX_2D> sph_;
    D3DXVECTOR2      case_offset_;
    D3DXVECTOR2      tmp_case_offset_;
    IConstraint*     constraint_;

};

#endif // WATER_HPP_
