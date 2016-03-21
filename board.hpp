// 2016/03/20 Naoyuki Hirayama

/*!
	@file	  board.hpp
	@brief	  <概要>

	<説明>
*/

#ifndef BOARD_HPP_
#define BOARD_HPP_

#include "trapezoidal_map.hpp"
#include "water.hpp"
#include "gci.hpp"
#include "pathview.hpp"
#include "color.hpp"
#include "castle.hpp"
#include "standard_partawn.hpp"
#include "station.hpp"
#include "basecamp.hpp"
#include "team.hpp"
#include <memory>

const int HCOUNT				   = 10;
const int VCOUNT				   = 10;

class Board {
public:
    Board()
        : tm_(0, 0, 1024, 1024), constraint_(terrain_, tmm_), ready_(false) {
    }

    void setup() {
        read_gci("data/cave.gci", terrain_);
        compile_terrain(terrain());
        // mockup();

        teams_.push_back(build_team(TeamTag::Alpha, Vector(64, 448)));
        teams_.push_back(build_team(TeamTag::Beta, Vector(448, 64)));
        ready_ = true;
    }

    void update(float elapsed) {
        for (const auto& team: teams_) {
            team->update(elapsed);
        }

        water_.update();

        for (const auto& team: teams_) {
            team->cleanup();
        }

        Castle::EmitEntry e;
        while (castle_.fetch(e)) {
            settle_partawn(e.team_tag, e.origin, e.target);
        }
    }

    bool ready() { return ready_; }

    const gci::Document& terrain() { return terrain_; }

    const std::vector<Primitive>& terrain_primitives() { 
        return terrain_primitives_; 
    }

    Water& water() { return water_; }

public: 
    // player operation
    std::shared_ptr<Team>
    build_team(TeamTag team_tag, const Vector& v) {
        // 拠点
        auto basecamp = std::make_shared<Basecamp>(team_tag, castle_, v);
        water_.add(v, MASS, basecamp.get());

        auto team = std::make_shared<Team>(castle_, team_tag, basecamp);
        return team;
    }

    std::shared_ptr<Team>
    team(TeamTag tag) { return teams_[(int)tag]; }        

    void settle_station(
        TeamTag team_tag, const Vector& origin, const Vector& target) {
        auto p = team(team_tag)->settle_station(origin, target);
        water_.add(origin, MASS, p.get());
    }

    void settle_partawn(
        TeamTag team_tag, const Vector& origin, const Vector& target) {
        auto p = team(team_tag)->settle_partawn(origin, target);
        water_.add(origin, MASS, p.get());
    }

    bool in_teritory(TeamTag team_tag, const Vector& v) {
        for (auto& s: team(team_tag)->stations()) {
            Vector d = s->location() - v;
            if (D3DXVec2Length(&d) < 50.0f) {
                return true;
            }
        }
        return false;
    }

public:
    struct SegmentProperty {
        int upper_cell_index;
        int lower_cell_index;
    };

private:
    class TrapezoidalMapConstraint : public IConstraint {
    public:
        TrapezoidalMapConstraint(
            gci::Document& doc,
            TrapezoidalMapMachine<float, SegmentProperty>& tmm)
            : document_(doc), tmm_(tmm) {}

        D3DXVECTOR2 apply(const D3DXVECTOR2& vv) {
            D3DXVECTOR2 v = vv;

            float minx = 1.0f;
            float miny = 1.0f;
            float maxx = 511.0f;
            float maxy = 511.0f;

            if (v.x <minx) { v.x = minx; }
            if (v.y <miny) { v.y = miny; }
            if (maxx <= v.x) { v.x = maxx; }
            if (maxy <= v.y) { v.y = maxy; }

            TrapezoidalMap<float, SegmentProperty>::Point q;
            int score;
            D3DXVECTOR2 qv(v.x, v.y);
            q.x(qv.x);
            q.y(qv.y);

            SegmentProperty tsp;
            SegmentProperty bsp;

            if (tmm_.find(q, score, tsp, bsp)) {
                if (0 <= tsp.lower_cell_index) {
                    const gci::Document::Cell& cell =
                        document_.voronoi_cells[tsp.lower_cell_index];

                    const gci::Document::Site& site =
                        document_.sites[cell.site_index];

                    D3DXVECTOR2 qv2;
                    if (site.is_segment) {
                        qv2 = nearest_point_on_line(
                            document_.vertices[site.p0],
                            document_.vertices[site.p1],
                            qv);
                    } else {
                        qv2 = document_.vertices[site.p0];
                    }

                    v =(qv2 - qv)* 0.5f + qv;
                }
            }

            return v;
        }

    private:
        D3DXVECTOR2 nearest_point_on_line(
            const D3DXVECTOR2& p0,
            const D3DXVECTOR2& p1,
            const D3DXVECTOR2& q) {
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            float a = dx * dx + dy * dy;
            if (a == 0) { return p0; }

            float b = dx *(p0.x - q.x)+ dy *(p0.y - q.y);
            float t = -(b / a);
            if (t <0.0f) { t = 0.0f; }
            if (1.0f <t) { t = 1.0f; }
            return D3DXVECTOR2(p0.x + dx * t, p0.y + dy * t);
        }

    private:
        gci::Document&                                      document_;
        TrapezoidalMapMachine<float, SegmentProperty>&      tmm_;
    };

    gci::Document terrain_;
    TrapezoidalMap<float, SegmentProperty> tm_;
    TrapezoidalMapMachine<float, SegmentProperty> tmm_;
    TrapezoidalMapConstraint constraint_;

    Castle      castle_;
    Water       water_;

    bool ready_;

    void compile_terrain(const gci::Document& doc) {
        // LINE
        Primitive p;
        p.opcode = Primitive::Color;
        p.operands[0] = 0;
        p.operands[1] = 0;
        p.operands[2] = 0;
        terrain_primitives_.push_back(p);

        for (size_t i = 0 ; i < doc.voronoi_cells.size() ; i++) {
            const std::vector<int>& v = doc.voronoi_cells[i].vertex_indices;
            Primitive p;
            p.opcode = Primitive::MoveTo;
            p.operands[0] = doc.vertices[v.back()].x;
            p.operands[1] = doc.vertices[v.back()].y;
            terrain_primitives_.push_back(p);
            for (size_t j = 0 ; j <v.size(); j++) {
                p.opcode = Primitive::LineTo;
                p.operands[0] = doc.vertices[v[j]].x;
                p.operands[1] = doc.vertices[v[j]].y;
                terrain_primitives_.push_back(p);
            }
        }

        // POLYGON
        int color_index = 0;
        for (size_t i = 0 ; i < doc.voronoi_cells.size(); i++) {
            post_random_color(terrain_primitives_, color_index);

            const std::vector<gci::Document::Triangle>& v =
                doc.voronoi_cells[i].triangles;

            for (size_t j = 0 ; j <v.size(); j++) {
                p.opcode = Primitive::MoveTo;
                p.operands[0] = doc.vertices[v[j].v0].x;
                p.operands[1] = doc.vertices[v[j].v0].y;
                terrain_primitives_.push_back(p);

                p.opcode = Primitive::Triangle;
                p.operands[0] = doc.vertices[v[j].v1].x;
                p.operands[1] = doc.vertices[v[j].v1].y;
                p.operands[2] = doc.vertices[v[j].v2].x;
                p.operands[3] = doc.vertices[v[j].v2].y;
                terrain_primitives_.push_back(p);
            }
        }

        // point location
        std::vector<TrapezoidalMap<float, SegmentProperty>::Point> 
            points;
        for (size_t i = 0 ; i < doc.vertices.size() ; i++) {
            points.push_back(tm_.make_point(doc.vertices[i].x,
                                            doc.vertices[i].y));
        }

        int m = 0;
        std::map<std::pair<int, int>, SegmentProperty> segments;
        for (size_t i = 0 ; i <doc.voronoi_cells.size() ; i++) {
            const std::vector<int>& v = doc.voronoi_cells[i].vertex_indices;

            for (size_t j = 0 ; j <v.size(); j++) {
                int index0 = v[j];
                int index1 = v[(j+1)% v.size()];

                // lexicographical compare
                bool invert = !(points[index0] <points[index1]);

                if (index1 <index0) { std::swap(index0, index1); }
                std::pair<int, int> s = std::make_pair(index0, index1);

                if (segments.find(s) == segments.end()) {
                    SegmentProperty sp;
                    sp.upper_cell_index = -1;
                    sp.lower_cell_index = -1;
                    if (invert) {
                        sp.upper_cell_index = int(i);
                    } else {
                        sp.lower_cell_index = int(i);
                    }
                    segments[s] = sp;
                    m++;
                    //tm_.add_segment(points[index0], points[index1]);
                } else {
                    SegmentProperty& sp = segments[s];
                    if (invert) {
                        sp.upper_cell_index = int(i);
                    } else {
                        sp.lower_cell_index = int(i);
                    }
                }
            }
        }

        struct Seg {
            int    index0;
            int    index1;
            SegmentProperty sp;
        };

        std::vector<Seg> segments2;
        for (const auto& p: segments){
            Seg seg;
            seg.index0 = p.first.first;
            seg.index1 = p.first.second;
            seg.sp = p.second;
            segments2.push_back(seg);
        }

        // シャッフル
        class Random {
        public:
            boost::mt19937 gen;
            boost::uniform_int<int> dst;
            boost::variate_generator<boost::mt19937, boost::uniform_int<int>>
                rand;
            Random(unsigned long seed, int N)
                : gen(seed), dst(0, N), rand(gen, dst) {
            }
            std::ptrdiff_t operator()(std::ptrdiff_t arg) {
                return static_cast<std::ptrdiff_t>(rand());
            }
        };

        //Random rnd((unsigned long)time(NULL), segments2.size());
        Random rnd(0, segments2.size());

        std::random_shuffle(segments2.begin(), segments2.end(), rnd);

        for (std::vector<Seg>::const_iterator i = segments2.begin();
             i != segments2.end();
             ++i) {
            tm_.add_segment(
                points[(*i).index0],
                points[(*i).index1],
                true,
                (*i).sp);
        }

        water_.set_constraint(&constraint_);

        tmm_.init(tm_);
    }

    std::vector<Command>    terrain_commands_; 
    std::vector<Primitive>  terrain_primitives_; 

    void post_random_color(std::vector<Primitive>& pb, int& color_index) {
        unsigned long color;
        do {
            color = get_color(color_index++);
        } while (get_color_distance(0xffffff, color)<0.1f);

        float r =((color & 0xff0000) >> 16)/ 255.0f;
        float g =((color & 0x00ff00) >>  8)/ 255.0f;
        float b =((color & 0x0000ff))/ 255.0f;

        Primitive p;
        p.opcode = Primitive::Color;
        p.operands[0] = r;
        p.operands[1] = g;
        p.operands[2] = b;
        pb.push_back(p);
    }

private:
    void mockup() {
    }

private:
    std::vector<std::shared_ptr<Team>> teams_;

};

#endif // BOARD_HPP_
