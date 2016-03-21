// 2008/12/24 Naoyuki Hirayama

/*!
	@file	  sph.hpp
	@brief	  <概要>

	<説明>
*/

#ifndef SPH_HPP_
#define SPH_HPP_

// 長さ mm
// 質量 g
// 時間 s

namespace sph {

namespace detail {

template <bool>
struct If  {
    template <class T, class U>
    struct Inner {
        typedef T type;
    };
};

template <>
struct If<false> {
    template <class T, class U>
    struct Inner {
        typedef U type;
    };
};

} // namespace detail

template <bool B, class T, class U>
struct If  {
    typedef typename detail::If <B>::template Inner<T, U>::type type;
};

template < class Traits >
class sph {
private:
    typedef typename Traits::real_type		real_type;
    typedef typename Traits::vector_type	vector_type;
    typedef typename Traits::load_type		load_type;

    struct Particle {
        Particle*	next;
        int		id;
		
        vector_type	new_position;
        vector_type	old_position;
        real_type	mass;
        real_type	pressure_balance;
        real_type	pressure_repulsive;
        real_type	density0;
        real_type	density_plain;
        real_type	density_balance;
        real_type	density_repulsive;
        real_type	density_balance_numerator;
        real_type	density_balance_denominator;
        real_type	density_balance_corrected;
        real_type	density_repulsive_numerator;
        real_type	density_repulsive_denominator;
        real_type	density_repulsive_corrected;
        real_type	boundariness;	// boundary if < 1.0f
        vector_type	move;
        load_type	load;
    };

    struct Pair {
        Particle*	car;
        Particle*	cdr;
        vector_type	diff;
        real_type	length_sq;
        real_type	length;
    };

    class HashTable {
    public:
        template < class V >
        HashTable( V& v )
        {
            memset( table_, 0, sizeof( table_ ) );

            for( typename V::iterator i = v.begin() ; i != v.end() ; ++i ) {
                Particle& pi = *i;

                int coord[Traits::DIMENSION];
                Traits::make_coords( coord, pi.new_position );
                size_t h = Traits::hash( coord, TABLE_SIZE );
                pi.next = table_[h];
                table_[h] = &pi;
            }	
        }
        ~HashTable() { }

        Particle* find(const int a[]) {
            return table_[Traits::hash(a, TABLE_SIZE)];
        }

    private:
        enum { TABLE_SIZE = 4997 };
        Particle*	table_[TABLE_SIZE];
    };
	
    static real_type kernelc() {
        return
            real_type(315.0)/
            (real_type(64.0)* real_type(3.1415926535));
    }

    static real_type kernel2(real_type length) {
        real_type x = real_type(1.0) - length;
        return x * x;
    }

    static real_type kernel3(real_type length) {
        real_type x = real_type(1.0) - length;
        return x * x * x;
    }

public:
    sph() {}
    ~sph() {}

    void initialize(
        real_type search_radius,
        real_type viscosity,
        real_type dumping,
        vector_type gravity,
        real_type ideal_density,
        real_type pressure_balance_coefficient,
        real_type pressure_repulsive_coefficient) {
        C_ = 0;

        // radiusが1.0になるようにパラメータを正規化
        //(9乘しているところとかあるので)
        src_search_radius_ = search_radius;
        viscosity_ = viscosity;
        dumping_ = dumping;
        gravity_ = gravity / search_radius;
        ideal_density_ = ideal_density;
        pressure_balance_coefficient_ = pressure_balance_coefficient;
        pressure_repulsive_coefficient_ = pressure_repulsive_coefficient;
    }

    void add_particle(const vector_type& v, real_type mass, load_type load) {
        Particle p;
        p.new_position = v / src_search_radius_;
        p.old_position = p.new_position;
        p.mass = mass;
        p.density0 = ideal_density_;
        p.density_balance = ideal_density_;
        p.density_repulsive = ideal_density_;
        p.move = Traits::zero_vector();
        p.load = load;
        particles_.push_back(p);
    }

    int pick(const vector_type& p, real_type range) {
        real_type maxlen = (std::numeric_limits<real_type>::max)();  

        int id = -1;
        for (size_t i = 0 ; i <particles_.size(); i++) {
            vector_type pos = particles_[i].new_position;

            vector_type diff = pos - p;
            real_type len = Traits::length_square(diff);
            if (len <maxlen) {
                maxlen = len;
                id = int(i);
            }
        }

        if (maxlen <= range) {
            return id;
        }
        return -1;
    }

    template <class F>
    void foreach(F f) {
        for (Particle& p: particles_) {
            f(
                p.id,
                p.new_position * src_search_radius_,
                p.mass,
                p.density_plain,
                p.density_balance_corrected,
                p.density_repulsive_corrected,
                p.boundariness,
                p.load);
        }
    }
	
    template <class F>
    void constraint(F f) {
        real_type i_src_search_radius = real_type(1)/ src_search_radius_;

        for (Particle& p: particles_) {
            p.new_position =
                f(p.new_position * src_search_radius_)
                * i_src_search_radius;
        }
    }

    template <class F>
    void discard(F f) {
        particles_.erase(
            std::remove_if(
                particles_.begin(),
                particles_.end(),
                [&](const Particle& p){ return f(p.load); }),
            particles_.end());
    }

    template <class F>
    void foreach_pair(F f) {
        for (Pair& p: pairs_) {
            f(p.car->load, p.cdr->load, p.length);
        }
    }
	
    void update(real_type dt) {
        C_ = 0;

        real_type idt = real_type(1.0) / dt;
        real_type i_src_search_radius = real_type(1.0) / src_search_radius_;

        // verlet integration
        for (size_t i = 0 ; i <particles_.size(); i++) {
            Particle& p = particles_[i];

            // use previous position to compute next velocity
            vector_type vdt = p.new_position - p.old_position;
            vector_type v = vdt * idt;

            p.new_position += 
                Traits::move(p.load, p.new_position * src_search_radius_) * 
                i_src_search_radius * dt;

            // save previous position
            p.old_position = p.new_position;

            // compute velocity
            vector_type fgrav = gravity_ * p.mass;
            vector_type a = fgrav / p.density_balance * dt;
            v += a;
            real_type speed = Traits::length(vdt);
            C_ =(std::max)(speed, C_);

            v = Traits::constraint_velocity(
                p.load, 
                v * src_search_radius_) * i_src_search_radius;

            // advance to predicted position
            p.new_position += v * dt * dumping_;
        }

        update_pairs();
        compute_plain_density();
        designate_boundary();
        compute_density();
        normalize_density();
        double_density_relaxation(dt);
    }

    void set_viscosity( real_type v ) { viscosity_ = v; }
    real_type get_viscosity() { return viscosity_; }
    void set_dumping( real_type d ) { dumping_ = d; }
    real_type get_dumping() { return dumping_; }

    void set_ideal_density(float x) {
        ideal_density_ = x;
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.density0 = x;
        }
    }

    float get_ideal_density() {
        return ideal_density_;
    }
						   
private:
    struct update_pairs_0 {
        static void exec(
            Particle& pi,
            HashTable& ht,
            const int coords[2][Traits::DIMENSION],
            int c[Traits::DIMENSION],
            std::vector<Pair>& pairs) {
            for (Particle* p = ht.find(c) ; p ; p = p->next) {
                Particle& pj = *p;
                if (&pj <= &pi) { continue; }

                vector_type v = pj.new_position - pi.new_position;
                real_type length_sq = Traits::length_sq(v);
                if (real_type(1.0) <= length_sq) { continue; }

                Pair pair;
                pair.car = &pi;
                pair.cdr = &pj;
                pair.diff = v;
                pair.length_sq = length_sq;
                pair.length = sqrt(length_sq);
                pairs.push_back(pair);
            }
        }
    };

    template <int M, int N>
    struct update_pairs_n {
        static void exec(
            Particle& pi,
            HashTable& ht,
            const int coords[2][Traits::DIMENSION],
            int c[Traits::DIMENSION],
            std::vector<Pair>& pairs) {
            for (int i = coords[0][N] ; i <= coords[1][N] ; i++) {
                c[N] = i;
                typename If <
                    M == N + 1,
                    update_pairs_0,
                    update_pairs_n<M, N+1>>::type::exec(
                        pi, ht, coords, c, pairs);
            }
        }
    };

    void update_pairs() {
        const real_type  r1 = real_type(1.0);
        const vector_type unit = Traits::unit_vector();

        HashTable ht(particles_);
        pairs_.clear();

        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];

            int coords[2][Traits::DIMENSION];
            Traits::make_coords(coords[0], pi.new_position - unit);
            Traits::make_coords(coords[1], pi.new_position + unit);

            // traverse 
            pi.density_plain  = 0;
            pi.density_balance  = 0;
            pi.density_repulsive = 0;

            int c[Traits::DIMENSION];
            typename update_pairs_n<Traits::DIMENSION, 0>::exec(
                pi, ht, coords, c, pairs_);
        }
    }
	
    void compute_plain_density() {
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.density_plain = kernelc();
        }

        for (int i = 0 ; i <int(pairs_.size()); i++) {
            Pair& pair = pairs_[i];
            Particle& pi = *pair.car;
            Particle& pj = *pair.cdr;

            real_type kernel = kernelc()* kernel3(pair.length_sq);
            pi.density_plain += kernel;
            pj.density_plain += kernel;
        }
    }
	
    void designate_boundary() {
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.boundariness = kernelc()/ pi.density_plain;
        }

        for (int i = 0 ; i <int(pairs_.size()); i++) {
            Pair& pair = pairs_[i];
            Particle& pi = *pair.car;
            Particle& pj = *pair.cdr;

            float kernel = kernelc()* kernel3(pair.length_sq);
            pi.boundariness += pj.mass * kernel / pj.density_plain;
            pj.boundariness += pi.mass * kernel / pi.density_plain;
        }
    }

    void compute_density() {
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.density_balance    = pi.mass;
            pi.density_repulsive  = pi.mass;
        }

        for (int i = 0 ; i <int(pairs_.size()); i++) {
            Pair& pair = pairs_[i];
            Particle& pi = *pair.car;
            Particle& pj = *pair.cdr;

            pi.density_balance += pj.mass * kernel2(pair.length);
            pi.density_repulsive += pj.mass * kernel3(pair.length);

            pj.density_balance += pi.mass * kernel2(pair.length);
            pj.density_repulsive += pi.mass * kernel3(pair.length);
        }
    }
	
    void normalize_density() {
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.density_balance_numerator = pi.mass;
            pi.density_balance_denominator = pi.mass / pi.density_balance;
            pi.density_repulsive_numerator = pi.mass;
            pi.density_repulsive_denominator = pi.mass / pi.density_repulsive;
        }

        for (int i = 0 ; i <int(pairs_.size()); i++) {
            Pair& pair = pairs_[i];
            Particle& pi = *pair.car;
            Particle& pj = *pair.cdr;

            real_type k2 = kernel2(pair.length);
            real_type k3 = kernel3(pair.length);

            if (real_type(1.0) <= pj.boundariness) {
                real_type b = pj.mass * k2;
                real_type r = pj.mass * k3;
                pi.density_balance_numerator   += b;
                pi.density_balance_denominator += b / pj.density_balance;
                pi.density_repulsive_numerator += r;
                pi.density_repulsive_denominator += r / pj.density_repulsive;
            }

            if (real_type(1.0) <= pi.boundariness) {
                real_type b = pi.mass * k2;
                real_type r = pi.mass * k3;
                pj.density_balance_numerator   += b;
                pj.density_balance_denominator += b / pi.density_balance;
                pj.density_repulsive_numerator += r;
                pj.density_repulsive_denominator += r / pi.density_repulsive;
            }
        }
    }
	
    void double_density_relaxation(float dt) {
        for (int i = 0 ; i <int(particles_.size()); i++) {
            Particle& pi = particles_[i];
            pi.move = Traits::zero_vector();

            if (Traits::epsilon() <= pi.density_balance_denominator) {
                pi.density_balance_corrected =
                    pi.density_balance_numerator /
                    pi.density_balance_denominator;
            } else {
                pi.density_balance_corrected = pi.density_balance;
            }

            if (Traits::epsilon() <= pi.density_repulsive_denominator) {
                pi.density_repulsive_corrected =
                    pi.density_repulsive_numerator /
                    pi.density_repulsive_denominator;
            } else {
                pi.density_repulsive_corrected = pi.density_repulsive;
            }

            pi.pressure_balance =
                pressure_balance_coefficient_ *
                (pi.density_balance_corrected - pi.density0);
            pi.pressure_repulsive =
                pressure_repulsive_coefficient_ *
                pi.density_repulsive_corrected;
        }

        for (int i = 0 ; i <int(pairs_.size()); i++) {
            Pair& pair = pairs_[i];
            Particle& pi = *pair.car;
            Particle& pj = *pair.cdr;

            vector_type v_n = pair.diff;
            if (Traits::epsilon()<pair.length) { v_n /= pair.length; }

            vector_type Di =
                square(dt)*
                (pi.pressure_balance *(1 - pair.length)+
                 pi.pressure_repulsive * square(1 - pair.length))*
                v_n;
            vector_type Dj =
                square(dt)*
                (pj.pressure_balance *(1 - pair.length)+
                 pj.pressure_repulsive * square(1 - pair.length))*
                -v_n;
            pj.new_position += Di / 2 - Dj / 2;
            pj.move += Di / 2 - Dj / 2;
            pi.new_position -= Di / 2 - Dj / 2;
            pi.move -= Di / 2 - Dj / 2;
        }
    }

private:
    inline real_type square( real_type x ) { return x * x; }

private:
    std::vector< Particle > particles_;
    std::vector< Pair >     pairs_;
    real_type               C_;
    real_type               src_search_radius_;
    real_type               viscosity_;
    real_type               dumping_;
    vector_type             gravity_;
    real_type               ideal_density_;
    real_type               pressure_balance_coefficient_;
    real_type               pressure_repulsive_coefficient_;

};

} // namespace sph

struct SPH_Traits_D3DX_2D {
    typedef float  real_type;
    typedef D3DXVECTOR2 vector_type;
    typedef struct {} load_type;
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

};

#endif // SPH_HPP_
