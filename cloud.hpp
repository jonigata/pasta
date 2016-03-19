// 2008/12/24 Naoyuki Hirayama

/*!
	@file	  sph.hpp
	@brief	  <äTóv>

	ä»ó™î≈sph âüÇµèoÇµÇµÇ©ÇµÇ»Ç¢
*/

#ifndef SPH_HPP_
#define SPH_HPP_

#include "performance_counter.hpp"

// í∑Ç≥ mm
// éøó  g
// éûä‘ s

namespace cloud {

template <class Traits>
class cloud {
private:
    typedef typename Traits::real_type      real_type;
    typedef typename Traits::vector_type    vector_type;
    typedef typename Traits::load_type      load_type;

    struct Particle {
        Particle* next;
        int   id;

        vector_type position;
        vector_type force;
        load_type load;
    };

    struct Pair {
        Particle* car;
        Particle* cdr;
        vector_type diff;
        real_type length_sq;
        real_type length;
    };

    class HashTable {
    public:
        template <class V>
        HashTable(V& v) {
            memset(table_, 0, sizeof(table_));

            for (Particle& pi: v) {
                int coord[Traits::DIMENSION];
                Traits::make_coords(coord, pi.position);
                size_t h = Traits::hash(coord, TABLE_SIZE);
                pi.next = table_[h];
                table_[h] = &pi;
            }
        }
        ~HashTable() {}

        Particle* find(const int a[]) {
            return table_[Traits::hash(a, TABLE_SIZE)];
        }

    private:
        enum { TABLE_SIZE = 4997 };
        Particle* table_[TABLE_SIZE];
    };

public:
    cloud() {}
    ~cloud() {}

    void initialize(real_type search_radius) {
        src_search_radius_ = search_radius;
    }

    void add_particle(const vector_type& v, const load_type& load) {
        Particle p;
        p.position = v / src_search_radius_;
        p.load = load;
        particles_.push_back(p);
    }

    int pick(const vector_type& pos, real_type range) {
        real_type maxlen =(std::numeric_limits<real_type>::max)();  

        auto i = std::min_element(
            particles_.cbegin(), 
            particles_.cend(),
            [](const Particle& p) {
                return Traits::length_square(p.position - pos);
            });
        if (i == particles_.cend() || maxlen < range) {
            return -1;
        }
        return id;
    }

    template <class F>
    void foreach(F f) {
        for (Particle& p: particles_) {
            f(p.id, p.position * src_search_radius_, p.load);
        }
    }

    template <class F>
    void constraint(F f) {
        real_type i_src_search_radius = real_type(1) / src_search_radius_;

        for (Particle& p: particles_) {
            p.position =
                f(p.position * src_search_radius_) * i_src_search_radius;
        }
    }

    void update(real_type dt) {
        update_pairs();

        for (Particle& p: particles_) {
            p.force = Traits::zero_vector();
        }

        for (Pair& p: pairs_) {
            if (p.length == 0) { continue; }  // Ç¢Ç¡ÇΩÇÒñ≥éã
            float rd = square(1.0f - p.length_sq);
            vector_type v = p.diff * (rd * dt * 60.0f);
            p.car->force -= v;
            p.cdr->force += v;
        }

        for (Particle& p: particles_) {
            p.position += p.force;
        }

    }

private:
    template <int M>
    struct update_pairs_n {
        template <int N>
        struct when_n {
            static void exec(
                Particle& pi,
                HashTable& ht,
                const int coords[2][M], // 2 means min, max
                int c[M],
                std::vector<Pair>& pairs) {
                for (int i = coords[0][M-N] ; i <= coords[1][M-N] ; i++) {
                    c[M-N] = i;
                    when_n<N-1>::exec(pi, ht, coords, c, pairs);
                }
            }
        };

        template <>
        struct when_n<0> {
            static void exec(
                Particle& pi,
                HashTable& ht,
                const int coords[2][M], // 2 means min, max
                int c[M],
                std::vector<Pair>& pairs) {
                for (Particle* p = ht.find(c); p ; p = p->next) {
                    Particle& pj = *p;
                    if (&pj <= &pi) { continue; }

                    vector_type v = pj.position - pi.position;
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
    };


    void update_pairs() {
        const vector_type diag = Traits::diag_vector();

        HashTable ht(particles_);
        pairs_.clear();

        for (Particle& p: particles_) {
            int coords[2][Traits::DIMENSION];
            Traits::make_coords(coords[0], p.position - diag);
            Traits::make_coords(coords[1], p.position + diag);

            int c[Traits::DIMENSION];
            typename
                update_pairs_n<Traits::DIMENSION>::
                when_n<Traits::DIMENSION>::exec(
                    p, ht, coords, c, pairs_);
        }
    }

    float square(float x) { return x * x; }

private:
    std::vector<Particle>   particles_;
    std::vector<Pair>       pairs_;
    real_type               src_search_radius_;

};

} // namespace sph

struct cloud_traits_D3DX_2D {
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
    static vector_type diag_vector() {
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
