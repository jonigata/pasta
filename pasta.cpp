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

#include "color.hpp"
#include "pathview.hpp"
#include "renderer.hpp"
#include "trapezoidal_map.hpp"
#include "water.hpp"
#include "gci.hpp"

////////////////////////////////////////////////////////////////
// CGAL
typedef CGAL::Gmpq                  ENT;
typedef CGAL::Lazy_exact_nt< ENT >  RT;
typedef CGAL::Simple_cartesian<RT>  EK;

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
    application()
        : tm_(0, 0, 1024, 1024), constraint_(document_, tmm_) {
        auto_update_ = false;
        step_ = 0;
        click_ = false;

        dragging_ = false;
    }
    ~application() {}

public:
    void on_timer(int elapsed0, float elapsed1) {
        d3d_user::on_timer(elapsed0, elapsed1);
        if (auto_update_ || click_) {
            constraint_.set_offset(
                -renderer_.get_tmp_offset()/ renderer_.get_zoom());
            water_.update();
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
        water_.render(
            device,
            //renderer_.get_tmp_offset(),
            D3DXVECTOR2(0, 0),
            renderer_.get_zoom());
        renderer_.render(device, primitives_, step_);
    }

    void accept(zw::win::event::create& m) {
        window_user_type::accept(m);

        font_vault.setup(d3d.device());

        read_gci("data/cave.gci", document_);
        draw_document(document_);
    }

    void accept(zw::win::event::size& m) {
        window_user_type::accept(m);
    }

    void accept(zw::win::event::paint& m) {
        window_user_type::accept(m);
    }

    void accept(zw::win::event::mouse& m) {
        window_user_type::accept(m);
    }

    void accept(zw::win::event::keychar& m) {
        if (m.code == VK_ESCAPE) {
            auto_update_ = !auto_update_;
        }
    }

private:
    void draw_document(const gci::Document& doc) {
        // LINE
        Primitive p;
        p.opcode = Primitive::Color;
        p.operands[0] = 0;
        p.operands[1] = 0;
        p.operands[2] = 0;
        primitives_.push_back(p);

        for (size_t i = 0 ; i < doc.voronoi_cells.size() ; i++) {
            const std::vector<int>& v = doc.voronoi_cells[i].vertex_indices;
            Primitive p;
            p.opcode = Primitive::MoveTo;
            p.operands[0] = doc.vertices[v.back()].x;
            p.operands[1] = doc.vertices[v.back()].y;
            primitives_.push_back(p);
            for (size_t j = 0 ; j <v.size(); j++) {
                p.opcode = Primitive::LineTo;
                p.operands[0] = doc.vertices[v[j]].x;
                p.operands[1] = doc.vertices[v[j]].y;
                primitives_.push_back(p);
            }
        }

        // POLYGON
        int color_index = 0;
        for (size_t i = 0 ; i < doc.voronoi_cells.size(); i++) {
            post_random_color(primitives_, color_index);

            const std::vector<gci::Document::Triangle>& v =
                doc.voronoi_cells[i].triangles;

            for (size_t j = 0 ; j <v.size(); j++) {
                p.opcode = Primitive::MoveTo;
                p.operands[0] = doc.vertices[v[j].v0].x;
                p.operands[1] = doc.vertices[v[j].v0].y;
                primitives_.push_back(p);

                p.opcode = Primitive::Triangle;
                p.operands[0] = doc.vertices[v[j].v1].x;
                p.operands[1] = doc.vertices[v[j].v1].y;
                p.operands[2] = doc.vertices[v[j].v2].x;
                p.operands[3] = doc.vertices[v[j].v2].y;
                primitives_.push_back(p);
            }
        }

        // AFTER ALL
        step_ = primitives_.size();

        // point location
        std::vector<TrapezoidalMap<float, SegmentProperty>::Point> points;
        for (size_t i = 0 ; i < doc.vertices.size() ; i++) {
            points.push_back(tm_.make_point(doc.vertices[i].x,
                                            doc.vertices[i].y));
        }

        int m = 0;
        std::map<std::pair<int, int>, SegmentProperty> segments;
        for (size_t i = 0 ; i <doc.voronoi_cells.size(); i++) {
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
        for (std::map<std::pair<int, int>, SegmentProperty>::const_iterator
                 i = segments.begin();
             i != segments.end();
             ++i) {
            Seg seg;
            seg.index0 =(*i).first.first;
            seg.index1 =(*i).first.second;
            seg.sp =(*i).second;
            segments2.push_back(seg);
        }

        // ƒVƒƒƒbƒtƒ‹
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

public:
    zw::font_vault    font_vault;
    zw::texture_vault texture_vault;

private:
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
    bool     auto_update_;
    bool     click_;
    int      step_;

    bool     dragging_;
    zw::win::offset_type dragstart_;

    std::vector<Command> commands_; 
    std::vector<Primitive> primitives_; 

    Renderer    renderer_;

    gci::Document   document_;

    struct SegmentProperty {
        int upper_cell_index;
        int lower_cell_index;
    };
    TrapezoidalMap<float, SegmentProperty> tm_;

    class TrapezoidalMapConstraint : public IConstraint {
    public:
        TrapezoidalMapConstraint(
            gci::Document& doc,
            TrapezoidalMapMachine<float, SegmentProperty>& tmm)
            : document_(doc), tmm_(tmm), offset_(0, 0) {}

        D3DXVECTOR2 apply(const D3DXVECTOR2& vv) {
            D3DXVECTOR2 v = vv;

            float minx = 1.0f + offset_.x;
            float miny = 1.0f + offset_.y;
            float maxx = 511.0f + offset_.x;
            float maxy = 511.0f + offset_.y;

            if (v.x <minx) { v.x = minx; }
            if (v.y <miny) { v.y = miny; }
            if (maxx <= v.x) { v.x = maxx; }
            if (maxy <= v.y) { v.y = maxy; }

            TrapezoidalMap<float, SegmentProperty>::Point q;
            int score;
            D3DXVECTOR2 qv(v.x - offset_.x, v.y - offset_.y);
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
                    v += offset_;
                }
            }

            return v;
        }

        void set_offset(const D3DXVECTOR2& v) { offset_ = v; }

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
        D3DXVECTOR2                                         offset_;
    };

    Water     water_;
    TrapezoidalMapMachine<float, SegmentProperty> tmm_;
    TrapezoidalMapConstraint constraint_;
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

