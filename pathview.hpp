// 2009/01/29 Naoyuki Hirayama

/*!
	@file	  pathview.hpp
	@brief	  <概要>

	ベクトルグラフィックスからGPUで扱いやすい中間表現への変換
*/

#ifndef PATHVIEW_HPP_
#define PATHVIEW_HPP_

////////////////////////////////////////////////////////////////
// datatype Primitive
struct Primitive {
    enum Opcode {
        Empty,
        Color,   // r g b
        MoveTo,   // x y
        LineTo,   // x y
        Triangle,  // x1 y1 x2 y2
        Dot,   // r
    } opcode;
    float operands[4];

    static 
    const char*
    to_string(Primitive::Opcode o) {
        switch (o) {
            case Primitive::Empty:  return "Primitive::Empty";
            case Primitive::Color:  return "Primitive::Color";
            case Primitive::MoveTo:  return "Primitive::MoveTo";
            case Primitive::LineTo:  return "Primitive::LineTo";
            case Primitive::Triangle: return "Primitive::Triangle";
            case Primitive::Dot:  return "Primitive::Dot";
            default:
                return "(nop)";
        }
    }

    const char*
    to_string() {
        return to_string(opcode);
    }

};

////////////////////////////////////////////////////////////////
// datatype Command
struct Command {
    enum Opcode {
        Empty,
        MoveTo,   // x y
        Close,   // e
        LineTo,   // x y
        Horizontal,  // x
        Vertical,  // y
        CurveTo,  // x1 y1 x2 y2 x y
        SmoothTo,  // x2 y2 x y
        QCurveTo,  // x1 y1 x y
        QSmoothTo,  // x y
        Arc,   // rx ry xaxis-rotation large-arc-flag sweep-flag x y

        Color,   // r g b
        Dot,   // r
        Triangle,  // x1 y1 x2 y2
        BeginFill,  // 
        EndFill,  // 
    } opcode;
    bool relative;
    float operands[8];

    static Command create(
        Command::Opcode opcode,
        bool   relative) {
        Command command;
        command.opcode = opcode;
        command.relative = relative;
        return command;
    }

    static const char* to_string(Command::Opcode o) {
        switch (o) {
            case Command::Empty:  return "Command::Empty";
            case Command::MoveTo:  return "Command::MoveTo";
            case Command::Close:  return "Command::Close";
            case Command::LineTo:  return "Command::LineTo";
            case Command::Horizontal: return "Command::Horizontal";
            case Command::Vertical:  return "Command::Vertical";
            case Command::CurveTo:  return "Command::CurveTo";
            case Command::SmoothTo:  return "Command::SmoothTo";
            case Command::QCurveTo:  return "Command::QCurveTo";
            case Command::QSmoothTo: return "Command::QSmoothTo";
            case Command::Arc:   return "Command::Arc";
            case Command::Color:  return "Command::Color";
            case Command::Dot:   return "Command::Dot";
            case Command::Triangle:  return "Command::Triangle";
            case Command::BeginFill: return "Command::BeginFill";
            case Command::EndFill:  return "Command::EndFill";
            default:
                return "(nop)";
        }
    }

    const char* to_string() {
        return to_string(opcode);
    }

};

////////////////////////////////////////////////////////////////
// CommmandBuilder
template <class T>
class CommandBuilder {
public:
    CommandBuilder(T& cb) : cb_(cb) {}
    ~CommandBuilder() {}

    void post(const Command& command) {
        if (command.opcode != Command::Empty) {
            cb_.push_back(command);
        }
    }

    template <class R>
    void post_color(R r, R g, R b) {
        Command c = Command::create(Command::Color, false);
        c.operands[0] = float(r);
        c.operands[1] = float(g);
        c.operands[2] = float(b);
        cb_.push_back(c);
    }

    template <class R>
    void post_move(R x, R y, bool relative = false) {
        Command c = Command::create(Command::MoveTo, relative);
        c.operands[0] = float(x);
        c.operands[1] = float(y);
        cb_.push_back(c);
    }

    template <class R>
    void post_line(R x, R y, bool relative = false) {
        Command c = Command::create(Command::LineTo, relative);
        c.operands[0] = float(x);
        c.operands[1] = float(y);
        cb_.push_back(c);
    }

    template <class R>
    void post_curveto(R x0, R y0, R x1, R y1, R x2, R y2,
                      bool relative = false) {
        Command c = Command::create(Command::CurveTo, relative);
        c.operands[0] = float(x0);
        c.operands[1] = float(y0);
        c.operands[2] = float(x1);
        c.operands[3] = float(y1);
        c.operands[4] = float(x2);
        c.operands[5] = float(y2);
        cb_.push_back(c);
    }

    template <class R>
    void post_dot(R r) {
        Command c = Command::create(Command::Dot, false);
        c.operands[0] = float(r);
        cb_.push_back(c);
    }

    template <class R>
    void post_triangle(R x1, R y1, R x2, R y2) {
        Command c = Command::create(Command::Triangle, false);
        c.operands[0] = float(x1);
        c.operands[1] = float(y1);
        c.operands[2] = float(x2);
        c.operands[3] = float(y2);
        cb_.push_back(c);
    }

    void post_beginfill() {
        Command c = Command::create(Command::BeginFill, false);
        cb_.push_back(c);
    }

    void post_endfill() {
        Command c = Command::create(Command::EndFill, false);
        cb_.push_back(c);
    }

    void post_close() {
        Command c = Command::create(Command::Close, false);
        cb_.push_back(c);
    }


private:
    T& cb_;

};

////////////////////////////////////////////////////////////////
// CommandCompiler
template <class Vec>
class CommandCompiler {
public:
    enum {
        BEZIER_DIVISION = 8,
        DOT_DIVISION = 32,
    };

    CommandCompiler() {}
    ~CommandCompiler() {}

    template <class CB, class PB>
    void compile(const CB& cb, PB& pb) {
        bool filling = false;
        Vec start;
        Vec cursor;

        std::vector<Vec> polygon;

        for (typename CB::const_iterator i = cb.begin();
             i != cb.end();
             ++i) {
            const Command& c = *i;
            const float* a = c.operands;

            switch (c.opcode) {
                case Command::Empty:
                    break;

                case Command::MoveTo:
                    if (c.relative) {
                        cursor += vec(a + 0);
                    } else {
                        cursor = vec(a + 0);
                    }

                    start = cursor;

                    if (filling) {
                        polygon.push_back(cursor);
                    } else {
                        Primitive p;
                        p.opcode = Primitive::MoveTo;
                        p.operands[0] = cursor.x;
                        p.operands[1] = cursor.y;
                        pb.push_back(p);
                    }
                    break;

                case Command::Close:
                    if (filling) {
                        //polygon.push_back(start);
                    } else {
                        Primitive p;
                        p.opcode = Primitive::LineTo;
                        p.operands[0] = start.x;
                        p.operands[1] = start.y;
                        pb.push_back(p);
                    }
                    break;

                case Command::LineTo:
                    if (c.relative) {
                        cursor += vec(a + 0);
                    } else {
                        cursor = vec(a + 0);
                    }
                    if (filling) {
                        polygon.push_back(cursor);
                    } else {
                        Primitive p;
                        p.opcode = Primitive::LineTo;
                        p.operands[0] = cursor.x;
                        p.operands[1] = cursor.y;
                        pb.push_back(p);
                    }
                    break;

                case Command::Horizontal:
                    if (c.relative) {
                        cursor.x += a[0];
                    } else {
                        cursor.x = a[0];
                    }
                    if (filling) {
                        polygon.push_back(cursor);
                    } else {
                        Primitive p;
                        p.opcode = Primitive::LineTo;
                        p.operands[0] = cursor.x;
                        p.operands[1] = cursor.y;
                        pb.push_back(p);
                    }
                    break;

                case Command::Vertical:
                    if (c.relative) {
                        cursor.y += a[0];
                    } else {
                        cursor.y = a[0];
                    }
                    if (filling) {
                        polygon.push_back(cursor);
                    } else {
                        Primitive p;
                        p.opcode = Primitive::LineTo;
                        p.operands[0] = cursor.x;
                        p.operands[1] = cursor.y;
                        pb.push_back(p);
                    }
                    break;

                case Command::CurveTo: {
                    Vec p0 = cursor;
                    Vec p1 = vec(a + 0);
                    Vec p2 = vec(a + 2);
                    Vec p3 = vec(a + 4);

                    if (c.relative) {
                        p1 += cursor;
                        p2 += cursor;
                        p3 += cursor;
                    }
                    if (filling) {
                        polygon_builder<std::vector<Vec>> pb(
                            polygon);
                        compile_curve(p0, p1, p2, p3, pb);
                    } else {
                        line_builder<std::vector<Primitive>> lb(pb);
                        compile_curve(p0, p1, p2, p3, lb);
                    }
                    cursor = p3;
                    break;
                }

                case Command::SmoothTo:
                case Command::QCurveTo:
                case Command::QSmoothTo:
                case Command::Arc:
                    break;

                case Command::Color: {
                    Primitive p;
                    p.opcode = Primitive::Color;
                    p.operands[0] = a[0];
                    p.operands[1] = a[1];
                    p.operands[2] = a[2];
                    pb.push_back(p);
                }
                    break;

                case Command::Dot:
                    //compile_dot(pb, cursor, a[0]);
                    {
                        Primitive p;
                        p.opcode = Primitive::Dot;
                        p.operands[0] = a[0];
                        pb.push_back(p);
                    }
                    break;

                case Command::BeginFill:
                    filling = true;
                    polygon.clear();
                    break;

                case Command::EndFill:
                    compile_polygon(pb, polygon);
                    filling = false;
                    break;

            }
        }
    }

    struct Polygon {
        bool    fill;
        boost::uint32_t  color;
        std::vector<Vec> vertices;
    };

    template <class CB, class PB>
    void make_polygons(const CB& cb, PB& pb) {
        Vec start;
        Vec cursor;

        Polygon polygon;
        for (typename CB::const_iterator i = cb.begin();
             i != cb.end();
             ++i) {
            const Command& c = *i;
            const float* a = c.operands;

            switch (c.opcode) {
                case Command::Empty:
                    break;

                case Command::MoveTo:
                    flush_polygon(polygon, pb);

                    if (c.relative) {
                        cursor += vec(a + 0);
                    } else {
                        cursor = vec(a + 0);
                    }

                    start = cursor;
                    polygon.vertices.push_back(cursor);
                    break;

                case Command::Close:
                    polygon.vertices.push_back(start);
                    break;

                case Command::LineTo:
                    if (c.relative) {
                        cursor += vec(a + 0);
                    } else {
                        cursor = vec(a + 0);
                    }

                    polygon.vertices.push_back(cursor);
                    break;

                case Command::Horizontal:
                    if (c.relative) {
                        cursor.x += a[0];
                    } else {
                        cursor.x = a[0];
                    }

                    polygon.vertices.push_back(cursor);
                    break;

                case Command::Vertical:
                    if (c.relative) {
                        cursor.y += a[0];
                    } else {
                        cursor.y = a[0];
                    }

                    polygon.vertices.push_back(cursor);
                    break;

                case Command::CurveTo: {
                    Vec p0 = cursor;
                    Vec p1 = vec(a + 0);
                    Vec p2 = vec(a + 2);
                    Vec p3 = vec(a + 4);

                    if (c.relative) {
                        p1 += cursor;
                        p2 += cursor;
                        p3 += cursor;
                    }

                    polygon_builder<std::vector<Vec>> pb(
                        polygon.vertices);
                    compile_curve(p0, p1, p2, p3, pb);
                    cursor = p3;
                    break;
                }

                case Command::SmoothTo:
                case Command::QCurveTo:
                case Command::QSmoothTo:
                case Command::Arc:
                    break;

                case Command::Color:
                    flush_polygon(polygon, pb);
                    polygon.vertices.clear();
                    polygon.color = ARGB(0.25, a[0], a[1], a[2]);
                    break;

                case Command::Dot:
                    break;

                case Command::BeginFill:
                    flush_polygon(polygon, pb);
                    polygon.fill = true;
                    break;

                case Command::EndFill:
                    flush_polygon(polygon, pb);
                    polygon.fill = false;
                    break;

            }
        }

    }

private:
    template <class PB>
    void flush_polygon(Polygon& polygon, PB& pb) {
        if (3 <= polygon.vertices.size()) {
            pb.push_back(polygon);
        }
        polygon.vertices.clear();
        polygon.fill = false;
    }

    Vec vec(const float* v) {
        return Vec(v[0], v[1]);
    }

    template <class T>
    struct line_builder {
        line_builder(T& v) : v_(v) {}
        T& v_;
        void operator()(const Vec& q) const {
            Primitive p;
            p.opcode = Primitive::LineTo;
            p.operands[0] = q.x;
            p.operands[1] = q.y;
            v_.push_back(p);
        }
    };

    template <class T>
    struct polygon_builder {
        polygon_builder(T& v) : v_(v) {}
        T& v_;
        void operator()(const Vec& q) const {
            v_.push_back(q);
        }
    };

    template <class F>
    void compile_curve(
        const Vec&   p0,
        const Vec&   p1,
        const Vec&   p2,
        const Vec&   p3,
        F       f) {
        Vec q0 = p0;
        for (int i = 0 ; i <BEZIER_DIVISION ; i++) {
            float t =(1.0f / BEZIER_DIVISION)*(i + 1);

            Vec q1 =
                p0 * powf(1.0f-t, 3.0f)+
                p1 *(3.0f * t * powf(1.0f-t, 2.0f))+
                p2 *(3.0f * t * t *(1.0f-t))+
                p3 *(t * t * t);

            f(q1);
            q0 = q1;
        }
    }

    template <class PB>
    void compile_dot(
        PB&     pb,
        const Vec& c,
        float    r) {
        for (int i = 0 ; i <DOT_DIVISION ; i++) {
            float t0 =(1.0f / DOT_DIVISION)*(i);
            float t1 =(1.0f / DOT_DIVISION)*(i + 1);

            t0 *= D3DX_PI * 2.0f;
            t1 *= D3DX_PI * 2.0f;

            float x0 = cosf(t0)* r;
            float y0 = sinf(t0)* r;
            float x1 = cosf(t1)* r;
            float y1 = sinf(t1)* r;

            Primitive p;
            if (i == 0) {
                p.opcode = Primitive::MoveTo;
                p.operands[0] = c.x + x0;
                p.operands[1] = c.y + y0;
            } else {
                p.opcode = Primitive::LineTo;
                p.operands[0] = c.x + x0;
                p.operands[1] = c.y + y0;
            }
            pb.push_back(p);
        }

        Primitive p;
        p.opcode = Primitive::MoveTo;
        p.operands[0] = c.x;
        p.operands[1] = c.y;
        pb.push_back(p);
    }

    template <class PB, class V>
    void compile_polygon(PB& pb, V& v) {
        typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
        typedef CGAL::Partition_traits_2<K>    Traits;
        typedef Traits::Point_2                         Point_2;
        typedef Traits::Polygon_2                       Polygon_2;
        typedef std::list<Polygon_2>                    Polygon_list;

        if (v.size()<3) { return; }

        unique(v);

        if (v.size()<3) { return; }

        Polygon_2    polygon;
        for (int i = 0 ; i <int(v.size()); i++) {
            polygon.push_back(Point_2(v[i].x, v[i].y));
        }

        if (polygon.is_clockwise_oriented()) {
            polygon.reverse_orientation();
        }

        if (!CGAL::is_simple_2(polygon.vertices_begin(), polygon.vertices_end())) {
            return;
        }

        Polygon_list partition_polys;
        CGAL::y_monotone_partition_2(
            polygon.vertices_begin(),
            polygon.vertices_end(),
            std::back_inserter(partition_polys));

        for (Polygon_list::iterator poly_it = partition_polys.begin();
             poly_it != partition_polys.end();
             poly_it++) {
            compile_monotone_polygon(pb, *poly_it);
        }
    }

    template <class T>
    void unique(T& v) {
        typename T::iterator b = v.begin();
        typename T::iterator e = v.end();

        for (typename T::iterator i ;(i = b) != e & ++b != e ;) {

            if (equal(*i, *b)) {
                while (++b != e) {
                    if (!equal(*i, *b)) {
                        *++i = *b;
                    }
                }
                v.erase(++i, v.end());
                break;
            }
        }

        if (equal(v.front(), v.back())) { v.pop_back(); }
    }

    template <class PB, class V>
    void compile_monotone_polygon(PB& pb, const V& v) {
        typedef typename V::Vertex_iterator Vertex_iterator;
        typedef CGAL::Circulator_from_iterator<Vertex_iterator>
            Vertex_circulator;

        // y最小の頂点を探す
        Vertex_iterator top = v.vertices_begin();
        Vertex_iterator i = top; 

        for (++i; i != v.vertices_end(); ++i) {
            if (i->y()<top->y()) {
                top = i;
            } else if (i->y() == top->y()) {
                if (i->x()<top->x()) {
                    top = i;
                }
            }
        }

        // ソート列を作る
        std::vector<std::pair<Vertex_circulator, int>> u;
        Vertex_circulator ctop(v.vertices_begin(), v.vertices_end(), top);
        Vertex_circulator car = ctop; --car;
        Vertex_circulator cdr = ctop; ++cdr;

        u.push_back(std::make_pair(ctop, 0));
        while (car != cdr) {
            if (car->y() == cdr->y()) {
                if (car->x()<cdr->x()) {
                    u.push_back(std::make_pair(car, -1)); --car;
                } else {
                    u.push_back(std::make_pair(cdr, +1)); ++cdr;
                }
            } else if (car->y()<cdr->y()) {
                u.push_back(std::make_pair(car, -1)); --car;
            } else {
                u.push_back(std::make_pair(cdr, +1)); ++cdr;
            }
        }
        u.push_back(std::make_pair(car, 0));  // == cdr

        for (int i = 0 ; i <int(u.size()); i++) {
            Primitive p;
            p.opcode = Primitive::MoveTo;
            p.operands[0] = float(u[i].first->x());
            p.operands[1] = float(u[i].first->y());
            pb.push_back(p);

            p.opcode = Primitive::Dot;
            p.operands[0] = 3;
            pb.push_back(p);
        }

        int color_index = 0;

        std::vector<std::pair<Vertex_circulator, int>> S;
        S.push_back(u[0]);
        S.push_back(u[1]);

        for (int j = 2 ; j <int(u.size())- 1 ; j++) {
            Primitive p;
            p.opcode = Primitive::MoveTo;
            p.operands[0] = float(u[j].first->x());
            p.operands[1] = float(u[j].first->y());
            pb.push_back(p);

            if (u[j].second != S.back().second) {
                // opposite chain

                for (int i = 1 ; i <int(S.size()); i++) {
                    // make diagonal
                    //post_random_color(pb, color_index);

                    p.opcode = Primitive::Triangle;
                    p.operands[0] = float(S[i-1].first->x());
                    p.operands[1] = float(S[i-1].first->y());
                    p.operands[2] = float(S[i].first->x());
                    p.operands[3] = float(S[i].first->y());
                    if (0 <u[j].second) {
                        std::swap(p.operands[0], p.operands[2]);
                        std::swap(p.operands[1], p.operands[3]);
                    }
                    pb.push_back(p);
                }
                S.erase(S.begin(), S.end()- 1);
                S.push_back(u[j]);
            } else {
                // same chain
                std::pair<Vertex_circulator, int> q = S.back();
                S.pop_back();

                while (!S.empty()&&
                       visible(
                           *u[j].first,
                           *q.first,
                           *S.back().first,
                           q.second)) {
                    // make diagonal
                    //post_random_color(pb, color_index);

                    p.opcode = Primitive::Triangle;
                    p.operands[0] = float(S.back().first->x());
                    p.operands[1] = float(S.back().first->y());
                    p.operands[2] = float(q.first->x());
                    p.operands[3] = float(q.first->y());
                    if (u[j].second <0) {
                        std::swap(p.operands[0], p.operands[2]);
                        std::swap(p.operands[1], p.operands[3]);
                    }
                    pb.push_back(p);

                    q = S.back();
                    S.pop_back();
                }
                S.push_back(q);
                S.push_back(u[j]);
            }
        }

        //post_random_color(pb, color_index);

        Primitive p;
        p.opcode = Primitive::MoveTo;
        p.operands[0] = float(u.back().first->x());
        p.operands[1] = float(u.back().first->y());
        pb.push_back(p);

        for (int j = 0 ; j <int(S.size())- 1 ; j++) {
            //post_random_color(pb, color_index);

            p.opcode = Primitive::Triangle;
            p.operands[0] = float(S[j].first->x());
            p.operands[1] = float(S[j].first->y());
            p.operands[2] = float(S[j+1].first->x());
            p.operands[3] = float(S[j+1].first->y());
            if (S[j+1].second <0) {
                std::swap(p.operands[0], p.operands[2]);
                std::swap(p.operands[1], p.operands[3]);
            }
            pb.push_back(p);
        }
    }

    bool equal(const Vec& a, const Vec& b) {
        //return D3DXVec2LengthSq(&(a - b))<0.01f;
        // TODO: 誤差でまくり
        return sq(a.x - b.x)+ sq(a.y - b.y)<0.01f;
    }

    template <class T>
    T sq(T x) { return x * x; }

    template <class Vertex>
    bool visible(
        const Vertex& a,
        const Vertex& b,
        const Vertex& c,
        int adv) {
        typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
        typedef CGAL::Partition_traits_2<K>    Traits;

        CGAL::Vector_2<K> v0 = c - a;
        CGAL::Vector_2<K> v1 = b - a;

        return 0 <(v0.x()* v1.y()- v0.y()* v1.x())* adv;
    }

    template <class PB>
    void post_random_color(PB& pv, int& color_index) {
        unsigned long color;
        do {
            color = get_color(color_index++);
        } while (get_color_distance(0xffffff, color)<0.3f);

        Primitive p;
        p.opcode = Primitive::Color;
        p.operands[0] =((color & 0xff0000)>> 16)/ 255.0f;
        p.operands[1] =((color & 0x00ff00)>>  8)/ 255.0f;
        p.operands[2] =((color & 0x0000ff))/ 255.0f;
        pv.push_back(p);
    }

    boost::uint32_t ARGB(float a, float r, float g, float b) {
        return
            ((int(a * 255)& 0xff)<< 24)+
            ((int(r * 255)& 0xff)<< 16)+
            ((int(g * 255)& 0xff)<< 8)+
            ((int(b * 255)& 0xff));
    }

};


#endif // PATHVIEW_HPP_
