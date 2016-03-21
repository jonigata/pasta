// 2008/10/27 Naoyuki Hirayama

/*!
	@file	  water.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef WATER_HPP_
#define WATER_HPP_

#include "zw/d3dfvf.hpp"
#include <boost/ref.hpp>
#include "sph.hpp"
#include "vector.hpp"

const float INITIAL_DISTANCE	   = 10.0f;	// 1cm(10mm)ŠÔŠu‚ÌŠiŽq‚ðì‚é
const float DT                     = 0.01f;	// 100ƒtƒŒ[ƒ€/s
const float SEARCH_RADIUS          = INITIAL_DISTANCE;
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

    virtual Vector apply(const Vector&) = 0;
};

class IPartawn {
public:
    virtual ~IPartawn() {}

    virtual Vector constraint_velocity(const Vector&) = 0;
    virtual Vector move(const Vector&) = 0;
    virtual void location(const Vector&) = 0;
    virtual float life() = 0;

    virtual void update(float elapsed) = 0;
};

struct WaterTraits {
    typedef float  real_type;
    typedef Vector vector_type;
    typedef IPartawn* load_type;
    enum { DIMENSION = 2 };

    static real_type epsilon() {
        return 1.0e-6f;
    }
    static vector_type zero_vector() {
        return vector_type(0.0f, 0.0f);
    }
    static vector_type unit_vector() {
        return vector_type(1.0f, 1.0f);
    }
    static real_type length(const vector_type& v) {
        return D3DXVec2Length(&v);
    }
    static real_type length_sq(const vector_type& v) {
        return D3DXVec2LengthSq(&v);
    }

    static int coord(real_type n) {
        return int(floor(n));
    }
    static void make_coords(int a[2], const vector_type& v) {
        a[0] = coord(v.x);
        a[1] = coord(v.y);
    }
    static void make_vector(vector_type& v, const real_type a[2]) {
        v.x = a[0];
        v.y = a[1];
    }
    static int hash(const int a[2], int table_size) {
        // large prime numbers
        const int p1 = 73856093;
        const int p2 = 19349663;

        return size_t((a[0] * p1)^(a[1] * p2))% table_size;
    }
    static Vector constraint_velocity(
        const load_type& load, const vector_type& v) {
        if (load == nullptr) { return v; }
        return load->constraint_velocity(v);
    }
    static Vector move(const load_type& load, const vector_type& p) {
        if (load == nullptr) { return p; }
        return load->move(p);
    }

};


class Water {
public:
    Water();
    ~Water() {}

    void add(const Vector& v, float mass, IPartawn* partawn);

    void render(LPDIRECT3DDEVICE9 device);

    void click(Vector& p) {}
    void update();

    void  set_viscosity(float);
    float  get_viscosity();
    void  set_dumping(float);
    float  get_dumping();
    void  set_ideal_density(float);
    float  get_ideal_density();

    void  set_constraint(IConstraint* constraint);

private:
    sph::sph<WaterTraits> sph_;
    IConstraint*     constraint_;

};

class TrivialPartawn : public IPartawn {
public:
    TrivialPartawn() { life_ = 1.0f; }

    void location(const Vector& v) { location_ = v; }
    const Vector& location() { return location_; }

    void life(float x) { life_ = x; }
    float life() { return life_; }

protected:
    Vector  location_;
    float   life_;
};

class ImmovablePartawn : public TrivialPartawn {
public:
    ImmovablePartawn(const Vector& origin) {
        location(origin);
    }

    Vector constraint_velocity(const Vector& av) { return Vector(0, 0); }
    Vector move(const Vector& p) { return Vector(0, 0); }

};

#endif // WATER_HPP_
