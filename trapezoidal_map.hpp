// 2009/02/27 Naoyuki Hirayama

/*!
	@file	  trapezoidal_map.hpp
	@brief	  <概要>

	<説明>
*/

#ifndef TRAPEZOIDAL_MAP_HPP_
#define TRAPEZOIDAL_MAP_HPP_

#include <vector>
#include <set>
#include <boost/lexical_cast.hpp>
#include "zw/dprintf.hpp"

template <class R, class SegmentProperty>
class TrapezoidalMap {
public:
    struct Point {
    public:
        Point() {}
        Point(R x, R y) : x_(x), y_(y) {}
        Point(const Point& r) : x_(r.x_), y_(r.y_) {}
        ~Point() {}

        Point& operator =(const Point& r) {
            x_ = r.x_;
            y_ = r.y_;
            return *this;
        }

        bool operator ==(const Point& r) const {
            return x_ == r.x_ && y_ == r.y_;
        }

        bool operator<(const Point& r) const {
            if (x_ <r.x_) { return true; }
            if (r.x_ <x_) { return false; }
            return y_ <r.y_;
        }

        R x()const { return x_; }
        R y()const { return y_; }
        void x(R ax) { x_ = ax; }
        void y(R ay) { y_ = ay; }

    private:
        R x_;
        R y_;

    };

    struct Segment {
    public:
        Segment(int id,
                const Point& p0, const Point& p1, bool swapped, bool border,
                const SegmentProperty& property)
            : id_(id),
              p0_(p0), p1_(p1), swapped_(swapped), border_(border),
              property_(property) {
            if (p0.x() != p1.x()) {
                a_ =(p1.y()- p0.y())/(p1.x()- p0.x());
                b_ = p0.y()- a_ * p0.x();
            } else {
                a_ = b_ = 0;
            }
            next_ = NULL;
            prev_ = NULL;
        }
        ~Segment() {}

        int get_id() { return id_; }

        const Point& p0() { return p0_; }
        const Point& p1() { return p1_; }
        bool swapped() { return swapped_; }
        bool border() { return border_; }

        R a() { return a_; }
        R b() { return b_; }

        Segment* prev() { return prev_; }
        Segment* next() { return next_; }
        void  prev(Segment* p) { prev_ = p; }
        void  next(Segment* p) { next_ = p; }

        bool is_vertical()const {
            return p0_.x() == p1_.x();
        }

        R calculate_y(R x) {
            assert(!is_vertical());
            if (x == p0_.x()) { return p0_.y(); }
            if (x == p1_.x()) { return p1_.y(); }
            return a_ * x + b_;
        }

        bool is_under_point(const Point& p) {
            // findから呼ばれる関数

            // 上下はy軸下向きで考えている

            if (is_vertical()) {
                if (p.y() < p1_.y()) { return true; }
                // 線に乗っている場合はfalse
                return false;
            } else {
                R sy = calculate_y(p.x());
                return p.y() < sy;
            }
        }

        bool is_under_segment(Segment* s) {
            // add_segmentから呼ばれる関数

            // 上下はy軸下向きで考えている

            if (is_vertical()) {
                if (s->is_vertical()) {
                    if (s->p1().y() <= p0_.y()) { return true; }
                    assert(p1_.y() <= s->p0().y()); // 失敗＝線分が重なってる
                    return false;
                } else {
                    if (s->p0().y() <= p0_.y()) { return true; }
                    assert(p1_.y() <= s->p0().y()); // 失敗＝線分が交差してる
                    return false;
                }
            } else {
                if (s->is_vertical()) {
                    if (s->p1().y() <= p0_.y()) { return true; }
                    assert(p0_.y() <= s->p0().y()); // 失敗＝線分が交差してる
                    return false;
                } else {
                    if (p0_ == s->p0()) {
                        return s->a() < a_;
                    }
                    return s->p0().y() < calculate_y(s->p0().x());
                }
            }
        }

        const SegmentProperty& property() { return property_; }

    private:
        int  id_;
        Point p0_;
        Point p1_;
        R  a_;
        R  b_;
        bool swapped_;
        bool border_;
        SegmentProperty property_;
        Segment* next_;
        Segment* prev_;

    };

private:
    class Leaf;
    class Glue;

    class Node {
    public:
        Node() {
            static int id_seed = 1;
            id_ = id_seed++;

            next_ = NULL;
            prev_ = NULL;
            addr_ = 0;
        }
        virtual ~Node() {}

        virtual Node* next_p(const Point& p, Leaf*& leaf) = 0;
        virtual Node* next_s(Segment* s, Leaf*& leaf) = 0;

        virtual Node* car() = 0;
        virtual Node* cdr() = 0;
        virtual Leaf* as_leaf() { return NULL; }
        virtual Glue* parent() { return NULL; }
        virtual void parent(Glue* parent) {}
        virtual Leaf* topleftmost() = 0;
        virtual Leaf* bottomleftmost() = 0;
        virtual void suture(Leaf*, Leaf*) = 0;
        virtual void collect_y(std::vector<R>&) = 0;
        virtual Leaf* leftmost(R y) = 0;

        virtual void draw(std::set<Node*>&, std::ostream&) {}

        int id() { return id_; }
        virtual std::string name() = 0;

        Node* prev() { return prev_; }
        Node* next() { return next_; }
        void prev(Node* p) { prev_ = p; }
        void next(Node* p) { next_ = p; }

        // make VM
    public:
        virtual void pass1(int& addr) = 0;
        virtual void pass2(char*) = 0;

        bool set_addr(int n) {
            if (0 <addr_) { return false; }
            addr_ = n;
            return true;
        }
        int get_addr() { return addr_; }

    private:
        int id_;
        Node*  next_;
        Node*  prev_;

        int   addr_;

    };

    class XNode : public Node {
    public:
        XNode(const Point& p) : car_(NULL), cdr_(NULL), p_(p) {}
        ~XNode() {}

        const Point& p() { return p_; }

        void connect(Node* a, Node* d) { car_ = a; cdr_ = d; }

        Node* next_p(const Point& p, Leaf*& leaf) {
            return next_aux(p, leaf);
        }

        Node* next_s(Segment* s, Leaf*& leaf) {
            return next_aux(s->p0(), leaf);
        }

        Node* car() { return car_; }
        Node* cdr() { return cdr_; }
        Leaf* topleftmost() {
            return car_ ? car_->topleftmost(): NULL;
        }
        Leaf* bottomleftmost() {
            return car_ ? car_->bottomleftmost(): NULL;
        }
        void suture(Leaf* ulm, Leaf* llm) {
            if (car_) { car_->suture(ulm, llm); }
            if (cdr_) { cdr_->suture(ulm, llm); }
        }
        void collect_y(std::vector<R>& v) {
            if (car_) { car_->collect_y(v); }
            if (cdr_) { cdr_->collect_y(v); }
        }
        Leaf* leftmost(R y) {
            if (car_) { return car_->leftmost(y); }
            if (cdr_) { return cdr_->leftmost(y); }
            assert(0); return NULL;
        }

        std::string name() {
            return "X" + boost::lexical_cast<std::string>(id());
        }

        void draw(std::set<Node*>& mark, std::ostream& os) {
            if (mark.find(this) != mark.end()) {
                return;
            }
            mark.insert(this);

            os << "  " << name()
               << "[label = \" { " << name()<< "\\n("
               << p_.x()<< ", " << p_.y()<< ")| { <left>left|<right>right } }"
               << "\"];" << std::endl;
            if (car_) {
                os << "  " << name()<< ":left -> " << car_->name()<< ";"
                   << std::endl;
                car_->draw(mark, os);
            }
            if (cdr_) {
                os << "  " << name()<< ":right -> " << cdr_->name()<< ";"
                   << std::endl;
                cdr_->draw(mark, os);
            }
        }

        void  pass1(int& addr) {
            if (!set_addr(addr)) { return; }
            addr += 20; // opcode, x, y, then, else
            if (car_) { car_->pass1(addr); }
            if (cdr_) { cdr_->pass1(addr); }
        }
        void pass2(char* b) {
            int addr = get_addr();
            char* p = b + get_addr();
            *((boost::uint32_t*)p) = 1;        p += 4;
            *((float*)p) = p_.x();         p += 4;
            *((float*)p) = p_.y();         p += 4;
            *((boost::uint32_t*)p) = car_ ? car_->get_addr(): 0; p += 4;
            *((boost::uint32_t*)p) = cdr_ ? cdr_->get_addr(): 0;
        }

    private:
        Node* next_aux(const Point& p, Leaf*& leaf) {
            if (p <p_) { return car_; }
            return cdr_;
        }

    private:
        Node* car_;
        Node* cdr_;
        Point p_;

    };

    class YNode : public Node {
    public:
        YNode(Segment* s) : car_(NULL), cdr_(NULL), s_(s) {}
        ~YNode() {}

        Segment* s() { return s_; }

        void connect(Node* a, Node* d) { car_ = a; cdr_ = d; }

        Node* next_p(const Point& p, Leaf*& leaf) {
            if (s_->is_under_point(p)) { return car_; }
            return cdr_;
        }

        Node* next_s(Segment* s, Leaf*& leaf) {
            if (s_->is_under_segment(s)) { return car_; }
            return cdr_;
        }

        Node* car() { return car_; }
        Node* cdr() { return cdr_; }
        Leaf* topleftmost() {
            return car_ ? car_->topleftmost(): NULL;
        }
        Leaf* bottomleftmost() {
            return cdr_ ?  cdr_->bottomleftmost(): NULL;
        }
        void suture(Leaf* ulm, Leaf* llm) {
            if (car_) {
                car_->suture(
                    ulm,
                    cdr_ ? cdr_->topleftmost(): NULL);
            }
            if (cdr_) {
                cdr_->suture(
                    car_ ? car_->bottomleftmost(): NULL,
                    llm);
            }
        }
        void collect_y(std::vector<R>& v) {
            v.push_back(s_->p0().y());
            v.push_back(s_->p1().y());
            if (car_) { car_->collect_y(v); }
            if (cdr_) { cdr_->collect_y(v); }
        }
        Leaf* leftmost(R y) {
            assert(y != s_->p0().y());
            if (y <s_->p0().y()) { return car_->leftmost(y); }
            return cdr_->leftmost(y);
        }

        std::string name() {
            return "Y" + boost::lexical_cast<std::string>(id());
        }

        void draw(std::set<Node*>& mark, std::ostream& os) {
            if (mark.find(this) != mark.end()) {
                return;
            }
            mark.insert(this);

            os << "  " << name()
               << "[label = \" { " << name()<< "\\n("
               << s()->p0().x()<< ", " << s()->p0().y()<< ")-("
               << s()->p1().x()<< ", " << s()->p1().y()
               << ")| { <upper>upper|<lower>lower } }"
               << "\"];" << std::endl;
            if (car_) {
                os << "  " << name()<< ":upper -> " << car_->name()<< ";"
                   << std::endl;
                car_->draw(mark, os);
            }
            if (cdr_) {
                os << "  " << name()<< ":lower -> " << cdr_->name()<< ";"
                   << std::endl;
                cdr_->draw(mark, os);
            }
        }

        void  pass1(int& addr) {
            if (!set_addr(addr)) { return; }
            addr += 36;// opcode, p0.x, p0.y, p1.x, p1.y, a, b, then, else
            if (car_) { car_->pass1(addr); }
            if (cdr_) { cdr_->pass1(addr); }
        }
        void pass2(char* b) {
            char* p = b + get_addr();
            *((boost::uint32_t*)p) = 2;        p += 4;
            *((float*)p) = s_->p0().x();       p += 4;
            *((float*)p) = s_->p0().y();       p += 4;
            *((float*)p) = s_->p1().x();       p += 4;
            *((float*)p) = s_->p1().y();       p += 4;
            *((float*)p) = s_->a();         p += 4;
            *((float*)p) = s_->b();         p += 4;
            *((boost::uint32_t*)p) = car_ ? car_->get_addr(): 0; p += 4;
            *((boost::uint32_t*)p) = cdr_ ? cdr_->get_addr(): 0;
        }

    private:
        Node*  car_;
        Node*  cdr_;
        Segment* s_;
    };

    class Glue : public Node {
    public:
        Glue(Node* p) : child_(p) { child_->parent(this); }
        ~Glue() {}

        Node* child() { return child_; }
        void child(Node* p) { child_ = p; child_->parent(this); }

        Node* next_p(const Point& p, Leaf*& leaf) { return child_; }
        Node* next_s(Segment* s, Leaf*& leaf) { return child_; }

        Node* car() { return child_; }
        Node* cdr() { return NULL; }
        Leaf* topleftmost() {
            return child_->topleftmost();
        }
        Leaf* bottomleftmost() {
            return child_->bottomleftmost();
        }
        void suture(Leaf* ulm, Leaf* llm) {
            child_->suture(ulm, llm);
        }
        void collect_y(std::vector<R>& v) {
            child_->collect_y(v);
        }
        Leaf* leftmost(R y) {
            return child_->leftmost(y);
        }

        std::string name() {
            return "S" + boost::lexical_cast<std::string>(id());
        }

        void draw(std::set<Node*>& mark, std::ostream& os) {
            if (mark.find(this) != mark.end()) {
                return;
            }
            mark.insert(this);

            os << "  " << name()<< " -> " << child_->name()<< ";"
               << std::endl;
            child_->draw(mark, os);
        }

        void  pass1(int& addr) {
            if (!set_addr(addr)) { return; }
            child_->pass1(addr);
        }
        void pass2(char*) {}

    private:
        Node* child_;

    };

    class Leaf : public Node { // = trapezoid
    public:
        Leaf() {
            upper_opposite_left_ = NULL;
            lower_opposite_left_ = NULL;
            mark_ = false;
            score_ = 0;
        }
        ~Leaf() {}

        int  score() { return score_; }
        void score(int v) { score_ = v; }

        void calculate_trapezoid(Point& p0, Point& p1, Point& p2, Point& p3) {
            p0.x(this->leftp.x());
            p1.x(this->rightp.x());
            p2.x(this->rightp.x());
            p3.x(this->leftp.x());
            p0.y(this->top->calculate_y(p0.x()));
            p1.y(this->top->calculate_y(p1.x()));
            p2.y(this->bottom->calculate_y(p2.x()));
            p3.y(this->bottom->calculate_y(p3.x()));
        }

        Glue* parent() { return parent_; }
        void parent(Glue* parent) { parent_ = parent; }

        Leaf* upper_opposite_left() { return upper_opposite_left_; }
        Leaf* lower_opposite_left() { return lower_opposite_left_; }

        Node* next_p(const Point& p, Leaf*& leaf) {
            leaf = this;
            return NULL;
        }

        Node* next_s(Segment* s, Leaf*& leaf) {
            leaf = this;
            return NULL;
        }

        Node* car() { return NULL; }
        Node* cdr() { return NULL; }
        Leaf* as_leaf() { return this; }
        Leaf* topleftmost() {
            if (upperleft) { return upperleft->topleftmost(); }
            return this;
        }
        Leaf* bottomleftmost() {
            if (lowerleft) { return lowerleft->bottomleftmost(); }
            return this;
        }
        void suture(Leaf* ulm, Leaf* llm) {
            upper_opposite_left_ = ulm;
            lower_opposite_left_ = llm;
        }
        void collect_y(std::vector<R>& v) {}
        Leaf* leftmost(R y) {
            return this;
        }

        std::string name() {
            return "L" + boost::lexical_cast<std::string>(id());
        }

        void draw(std::set<Node*>&, std::ostream&) {
        }

        template <class T>
        void draw_leaf(T& visitor) {
            Point p0, p1, p2, p3;
            calculate_trapezoid(p0, p1, p2, p3);
            visitor(p0, p1, p2, p3, score_);
        }
        template <class T>
        void draw_strict(T& visitor) {
            std::vector<Point> v;
            Point p0, p1, p2, p3;
            calculate_trapezoid(p0, p1, p2, p3);

            // p0-p1
            v.push_back(p0);
            Leaf* q = upper_opposite_left_;
            if (q) {
                while (q->rightp.x() <= p0.x()) {
                    q = q->lowerright;
                }
                while (q->rightp.x()<p1.x()) {
                    R x = q->rightp.x();
                    R y = top->calculate_y(x);
                    v.push_back(Point(x, y));
                    q = q->lowerright;
                }
            }
            v.push_back(p1);

            if (!(p1 == p2)) {
                // p1-p2
                if (!(top->p1() == rightp)&& !(bottom->p1() == rightp)) {
                    v.push_back(rightp);
                }
                v.push_back(p2);
            }

            // p2-p3
            q = lower_opposite_left_;
            if (q) {
                while (q->rightp.x()<p2.x()) {
                    q = q->upperright;
                }
                while (p3.x()<q->leftp.x()) {
                    R x = q->leftp.x();
                    R y = bottom->calculate_y(x);
                    v.push_back(Point(x, y));
                    q = q->upperleft;
                }
            }

            // p3-p0
            if (!(p3 == p0)) {
                v.push_back(p3);
                if (!(top->p0() == leftp)&& !(bottom->p0() == leftp)) {
                    v.push_back(leftp);
                }
            }

            visitor(v, score_);
        }

        Segment* top;
        Segment* bottom;
        Point  leftp;
        Point  rightp;
        Leaf*  upperleft;
        Leaf*  lowerleft;
        Leaf*  upperright;
        Leaf*  lowerright;

        void  pass1(int& addr) {
            set_addr(addr);
            addr += 64 + sizeof(SegmentProperty)* 2;
        }
        void pass2(char* b) {
            char* p = b + get_addr();
            *((boost::uint32_t*)p) = 3;        p += 4;
            *((float*)p) = top->p0().x();       p += 4;
            *((float*)p) = top->p0().y();       p += 4;
            *((float*)p) = top->p1().x();       p += 4;
            *((float*)p) = top->p1().y();       p += 4;
            *((float*)p) = top->a();        p += 4;
            *((float*)p) = top->b();        p += 4;
            *((float*)p) = bottom->p0().x();      p += 4;
            *((float*)p) = bottom->p0().y();      p += 4;
            *((float*)p) = bottom->p1().x();      p += 4;
            *((float*)p) = bottom->p1().y();      p += 4;
            *((float*)p) = bottom->a();        p += 4;
            *((float*)p) = bottom->b();        p += 4;
            *((float*)p) = leftp.x();        p += 4;
            *((float*)p) = rightp.x();        p += 4;
            *((boost::uint32_t*)p) = score_;      p += 4;

            *((SegmentProperty*)p) = top->property();
            p += sizeof(SegmentProperty);
            *((SegmentProperty*)p) = bottom->property();
        }

    private:
        Glue*  parent_;
        Leaf*  upper_opposite_left_;
        Leaf*  lower_opposite_left_;
        bool  mark_;
        int   score_;

    };

public:
    TrapezoidalMap(
        R bbminx, R bbminy,
        R bbmaxx, R bbmaxy) {
        segment_id_seed_ = 0;
        node_chain_ = NULL;
        segment_chain_ = NULL;

        lowest_score_ = 0;
        highest_score_ = 0;

        Point p0(bbminx, bbminy);
        Point p1(bbmaxx, bbminy);
        Point p2(bbminx, bbmaxy);
        Point p3(bbmaxx, bbmaxy);
        Segment* s0 = new_segment(p0, p1, false, false, SegmentProperty());
        Segment* s1 = new_segment(p2, p3, false, false, SegmentProperty());
        Leaf* leaf = new_leaf();
        leaf->top = s0;
        leaf->bottom = s1;
        leaf->leftp = p2;
        leaf->rightp = p1;
        leaf->upperleft = NULL;
        leaf->lowerleft = NULL;
        leaf->upperright = NULL;
        leaf->lowerright = NULL;
        Glue* glue = link_node(new Glue(leaf));

        tree_ = glue;
    }
    ~TrapezoidalMap() { {
            Segment* p = segment_chain_;
            while (p) {
                Segment* q = p->next();
                delete p;
                p = q;
            }
        } {
            Node* p = node_chain_;
            while (p) {
                Node* q = p->next();
                delete p;
                p = q;
            }
        }
    }

    void freeze() {
        tree_->suture(NULL, NULL);
        set_leaf_scores();
    }

    Point make_point(R x, R y) const {
        return Point(x, y);
    }

    void add_segment(Point p0, Point p1,
                     bool border = true,
                     const SegmentProperty& property = SegmentProperty()) {
#if 0
        dprintf("insert segment:(%f, %f)-(%f, %f)\n",
                p0.x(), p0.y(), p1.x(), p1.y());

        static int mm = 0;
        mm++;

        check_validity();

        output_dot("t" + boost::lexical_cast<std::string>(mm)+ ".dot");
#endif

        bool swapped = false;
        if (p1.x()<p0.x()||
            (p1.x() == p0.x()&& p1.y()<p0.y())) {
            std::swap(p0, p1);
            swapped = true;
        }

        Segment* s = new_segment(p0, p1, swapped, border, property);

        Leaf* leaf = find_leaf(s);

        std::vector<Leaf*> v;
        follow_segment(s, v, leaf);
        if (v.size() == 1) {
            Leaf* t = leaf;
            Leaf* A = p0 == t->leftp ? NULL : new_leaf();
            Leaf* B = p1 == t->rightp ? NULL : new_leaf();
            Leaf* C = new_leaf();
            Leaf* D = new_leaf();
            if (A) {
                A->upperleft = t->upperleft;
                A->lowerleft = t->lowerleft;
                A->upperright = C;
                A->lowerright = D;
                C->upperleft = A;
                C->lowerleft = NULL;
                D->upperleft = NULL;
                D->lowerleft = A;
                A->top = t->top;
                A->bottom = t->bottom;
                A->leftp = t->leftp;
                A->rightp = p0;
            } else {
                C->upperleft = t->upperleft;
                C->lowerleft = NULL;
                D->upperleft = NULL;
                D->lowerleft = t->lowerleft;
            }
            C->top = t->top;
            C->bottom = s;
            C->leftp = p0;
            C->rightp = p1;
            D->top = s;
            D->bottom = t->bottom;
            D->leftp = p0;
            D->rightp = p1;
            if (B) {
                C->upperright = B;
                C->lowerright = NULL;
                D->upperright = NULL;
                D->lowerright = B;
                B->upperleft = C;
                B->lowerleft = D;
                B->upperright = t->upperright;
                B->lowerright = t->lowerright;
                B->top = t->top;
                B->bottom = t->bottom;
                B->leftp = p1;
                B->rightp = t->rightp;
            } else {
                C->upperright = t->upperright;
                C->lowerright = NULL;
                D->upperright = NULL;
                D->lowerright = t->lowerright;
            }
            if (t->upperleft) { t->upperleft->upperright = A ? A : C; }
            if (t->lowerleft) { t->lowerleft->lowerright = A ? A : D; }
            if (t->upperright) { t->upperright->upperleft = B ? B : C; }
            if (t->lowerright) { t->lowerright->lowerleft = B ? B : D; }
            Node* n;
            YNode* si = link_node(new YNode(s));
            si->connect(link_node(new Glue(C)),
                        link_node(new Glue(D)));
            n = si;
            if (B) {
                XNode* qi = link_node(new XNode(p1));
                qi->connect(n, link_node(new Glue(B)));
                n = qi;
            }
            if (A) {
                XNode* pi = link_node(new XNode(p0));
                pi->connect(link_node(new Glue(A)), n);
                n = pi;
            }
            replace(t, n);
            delete_leaf(t);
        } else { // 1 <v.size()
            Leaf* t = leaf;
            Leaf* u = new_leaf();
            Leaf* d = new_leaf();
            Glue* su = link_node(new Glue(u));
            Glue* sd = link_node(new Glue(d));
            u->top = t->top;
            u->bottom = s;
            d->top = s;
            d->bottom = t->bottom;
            if (t->leftp == p0) {
                u->upperleft = t->upperleft;
                u->lowerleft = NULL;
                d->upperleft = NULL;
                d->lowerleft = t->lowerleft;
                if (t->upperleft) { t->upperleft->upperright = u; }
                if (t->lowerleft) { t->lowerleft->lowerright = d; }
                YNode* si = link_node(new YNode(s));
                si->connect(su, sd);
                replace(t, si);
            } else {
                Leaf* l = new_leaf();
                l->upperleft = t->upperleft;
                l->lowerleft = t->lowerleft;
                l->upperright = u;
                l->lowerright = d;
                l->top = t->top;
                l->bottom = t->bottom;
                l->leftp = t->leftp;
                l->rightp = p0;
                u->upperleft = l;
                u->lowerleft = NULL;
                d->upperleft = NULL;
                d->lowerleft = l;
                if (t->upperleft) { t->upperleft->upperright = l; }
                if (t->lowerleft) { t->lowerleft->lowerright = l; }
                YNode* si = link_node(new YNode(s));
                si->connect(su, sd);
                XNode* qi = link_node(new XNode(p0));
                qi->connect(link_node(new Glue(l)), si);
                replace(t, qi);
            }
            u->leftp = p0;
            d->leftp = p0;
            for (int i = 1 ; i <int(v.size()); i++) {
                t = v[i];
                if (v[i-1]->upperright == t) {
                    Leaf* k = new_leaf();
                    d->upperright = k;
                    d->lowerright = v[i-1]->lowerright;
                    if (d->lowerright) { d->lowerright->lowerleft = d; }
                    d->rightp = t->leftp;
                    k->upperleft = d;
                    k->lowerleft = t->lowerleft;
                    k->top = s;
                    k->bottom = t->bottom;
                    k->leftp = d->rightp;
                    if (t->lowerleft) { t->lowerleft->lowerright = k; }
                    if (t->upperright) { t->upperright->upperleft = k; }
                    if (t->lowerright) { t->lowerright->lowerleft = k; }
                    d = k;
                    sd = link_node(new Glue(d));
                } else {
                    assert(v[i-1]->lowerright == t);
                    Leaf* k = new_leaf();
                    u->upperright = v[i-1]->upperright;
                    if (u->upperright) { u->upperright->upperleft = u; }
                    u->lowerright = k;
                    u->rightp = t->leftp;
                    k->upperleft = t->upperleft;
                    k->lowerleft = u;
                    k->top = t->top;
                    k->bottom = s;
                    k->leftp = u->rightp;
                    if (t->upperleft) { t->upperleft->upperright = k; }
                    if (t->upperright) { t->upperright->upperleft = k; }
                    if (t->lowerright) { t->lowerright->lowerleft = k; }
                    u = k;
                    su = link_node(new Glue(u));
                }
                if (i <int(v.size()- 1)) {
                    YNode* si = link_node(new YNode(s));
                    si->connect(su, sd);
                    replace(t, si);
                }
            }
            t = v.back();
            u->rightp = p1;
            d->rightp = p1;
            if (t->rightp == p1) {
                u->upperright = t->upperright;
                u->lowerright = NULL;
                d->upperright = NULL;
                d->lowerright = t->lowerright;
                if (t->upperright) { t->upperright->upperleft = u; }
                if (t->lowerright) { t->lowerright->lowerleft = d; }
                YNode* si = link_node(new YNode(s));
                si->connect(su, sd);
                replace(t, si);
            } else {
                Leaf* r = new_leaf();
                u->upperright = r;
                u->lowerright = NULL;
                d->upperright = NULL;
                d->lowerright = r;
                r->upperleft = u;
                r->lowerleft = d;
                r->upperright = t->upperright;
                r->lowerright = t->lowerright;
                r->top = t->top;
                r->bottom = t->bottom;
                r->leftp = p1;
                r->rightp = t->rightp;
                if (t->upperright) { t->upperright->upperleft = r; }
                if (t->lowerright) { t->lowerright->lowerleft = r; }
                YNode* si = link_node(new YNode(s));
                si->connect(su, sd);
                XNode* qi = link_node(new XNode(p1));
                qi->connect(si, link_node(new Glue(r)));
                replace(t, qi);
            }

            for (size_t iii = 0 ; iii <v.size(); iii++) {
                delete_leaf(v[iii]);
            }
        }

#if 0
        check_validity();
#endif
    }

    bool find(
        const Point& q,
        Point& p0,
        Point& p1,
        Point& p2,
        Point& p3,
        int& score) const {
        Leaf* leaf = find_leaf(q);
        if (!leaf) { return false; } // 範囲外
        leaf->calculate_trapezoid(p0, p1, p2, p3);
        score = leaf->score();
#if 0
        dprintf("found: L%d\n", leaf->id());
#endif

        return true;
    }

    bool find(
        const Point& q,
        Point& p0,
        Point& p1,
        Point& p2,
        Point& p3,
        int& score,
        SegmentProperty& top_segment_property,
        SegmentProperty& bottom_segment_property) const {
        Leaf* leaf = find_leaf(q);
        if (!leaf) { return false; } // 範囲外
        leaf->calculate_trapezoid(p0, p1, p2, p3);
        score = leaf->score();
        top_segment_property = leaf->top->property();
        bottom_segment_property = leaf->bottom->property();
#if 0
        dprintf("found: L%d\n", leaf->id());
#endif

        return true;
    }

    bool find(
        const Point& q,
        int& score,
        SegmentProperty& top_segment_property,
        SegmentProperty& bottom_segment_property) const {
        Leaf* leaf = find_leaf(q);
        if (!leaf) { return false; } // 範囲外
        score = leaf->score();
        top_segment_property = leaf->top->property();
        bottom_segment_property = leaf->bottom->property();
#if 0
        dprintf("found: L%d\n", leaf->id());
#endif

        return true;
    }

    int get_score(const Point& q) {
        Leaf* leaf = find_leaf(&q);
        if (!leaf) { return 0; } // 範囲外
        return chase_horizontal(leaf);
    }

    int get_lowest_score()const { return lowest_score_; }
    int get_highest_score()const { return highest_score_; }

    template <class T>
    void draw(T& t) const {
        for (Node* p = node_chain_ ; p != NULL ; p = p->next()) {
            Leaf* leaf = p->as_leaf();
            if (leaf) {
                leaf->draw_leaf(t);
            }
        }
    }

    template <class T>
    void draw_strict(T& t) const {
        for (Node* p = node_chain_ ; p != NULL ; p = p->next()) {
            Leaf* leaf = p->as_leaf();
            if (leaf) {
                leaf->draw_strict(t);
            }
        }
    }

    void compile(std::vector<char>& v) const {
        assert(sizeof(boost::uint32_t) == 4);
        assert(sizeof(float) == 4);

        int addr = 4;
        tree_->pass1(addr);

        v.resize(addr);
        for (Node* p = node_chain_ ; p != NULL ; p = p->next()) {
            p->pass2(&v[0]);
        }
    }

private:
    void replace(Leaf* t, Node* pi) {
        t->parent()->child(pi);
    }

    Leaf* find_leaf(const Point& p) const {
        Node* curr = tree_;
        for (;;) {
            Leaf* leaf = NULL;
            curr = curr->next_p(p, leaf);
            if (!curr) { return leaf; }
        }
    }

    Leaf* find_leaf(Segment* s) {
        Node* curr = tree_;
        for (;;) {
            Leaf* leaf = NULL;
            curr = curr->next_s(s, leaf);
            if (!curr) { return leaf; }
        }
    }

    void follow_segment(Segment* s, std::vector<Leaf*>& v, Leaf* p) {
        for (;;) {
            v.push_back(p);
            if (s->p1().x() <= p->rightp.x()) {
                break;
            }
            if (s->is_under_point(p->rightp)) {
                p = p->lowerright;
                assert(p); // perhaps segment intersection
            } else {
                p = p->upperright;
                assert(p); // perhaps segment intersection
            }
        }
    }

    int chase_horizontal(Leaf* leaf) {
        R x0 = leaf->leftp.x();
        R x1 = leaf->rightp.x();
        R y0 = leaf->top->calculate_y(x0);
        R y1 = leaf->bottom->calculate_y(x1);
        R cx =(x0 + x1)* R(0.5);
        R cy =(y0 + y1)* R(0.5);

        int score = 0;

        Leaf* p = leaf;
        while (p) {
            if (cy <p->top->calculate_y(p->rightp.x())) {
                if (p->top->border()) {
                    score += p->top->swapped()? 1 : -1;
                }

                // cross top segment
                Leaf* q = p->upper_opposite_left();
                while (q && q->rightp &&
                       q->bottom->calculate_y(q->rightp.x())<cy) {
                    q = q->lowerright;
                }
                p = q;
            } else if (p->bottom->calculate_y(p->rightp.x())<cy) {
                if (p->bottom->border()) {
                    score -= p->bottom->swapped()? 1 : -1;
                }

                // cross bottom segment
                Leaf* q = p->lower_opposite_left();
                while (q && q->rightp &&
                       cy <q->top->calculate_y(q->rightp.x())) {
                    q = q->lowerright;
                }
                p = q;
            } else {
                // not cross
                if (p->rightp == p->bottom->p1()) {
                    p = p->upperright;
                } else if (p->rightp == p->top->p1()) {
                    p = p->lowerright;
                } else {
                    if (cy <p->rightp.y()) {
                        p = p->upperright;
                    } else if (p->rightp.y()<cy) {
                        p = p->lowerright;
                    } else {
                        if (0 <p->upperright->bottom->a()) {
                            p = p->lowerright;
                        } else if (p->upperright->bottom->a()<0) {
                            p = p->upperright;
                        } else {
                            //線分と完全に重なっている
                            p = p->upperright; // どっちでもいい
                        }
                    }
                }
            }
        }
        return score;
    }

    void set_leaf_scores() {
        lowest_score_ = 0;
        highest_score_ = 0;

        std::vector<R> v;
        tree_->collect_y(v);
        if (v.empty()) { return; }

        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());

        for (size_t i = 0 ; i <v.size()- 1 ; i++) {
            R cy =(v[i] + v[i+1])* R(0.5);
            if (cy == v[i] || cy == v[i+1]) { continue; }

            // 最左ノード
            Leaf* leaf = tree_->leftmost(cy);
            assert(!leaf->upperleft);
            assert(!leaf->lowerleft);

            int score = 0;

            Leaf* p = leaf;
            while (p) {
                p->score(score);
                if (score <lowest_score_) {
                    lowest_score_ = score;
                }
                if (highest_score_ <score) {
                    highest_score_ = score;
                }

                if (!p->upperright &&
                    !p->lowerright &&
                    !p->upper_opposite_left()&&
                    !p->lower_opposite_left()) {
                    // rightmost
                    break;
                }

                if (cy <p->top->calculate_y(p->rightp.x())) {
                    if (p->top->border()) {
                        score += p->top->swapped()? 1 : -1;
                    }

                    // cross top segment
                    Leaf* q = p->upper_opposite_left();
                    while (q &&
                           q->bottom->calculate_y(q->rightp.x())<cy) {
                        q = q->lowerright;
                    }
                    p = q;
                } else if (p->bottom->calculate_y(p->rightp.x())<cy) {
                    if (p->bottom->border()) {
                        score -= p->bottom->swapped()? 1 : -1;
                    }

                    // cross bottom segment
                    Leaf* q = p->lower_opposite_left();
                    while (q &&
                           cy <q->top->calculate_y(q->rightp.x())) {
                        q = q->lowerright;
                    }
                    p = q;
                } else {
                    // not cross
                    if (p->rightp == p->bottom->p1()) {
                        p = p->upperright;
                    } else if (p->rightp == p->top->p1()) {
                        p = p->lowerright;
                    } else {
                        if (cy <p->rightp.y()) {
                            p = p->upperright;
                        } else if (p->rightp.y()<cy) {
                            p = p->lowerright;
                        } else {
                            if (0 <p->upperright->bottom->a()) {
                                p = p->lowerright;
                            } else if (p->upperright->bottom->a()<0) {
                                p = p->upperright;
                            } else {
                                //線分と完全に重なっている
                                p = p->upperright; // どっちでもいい
                            }
                        }
                    }
                }
            }
        }
    }

    bool is_internal_trapezoid(Leaf* p) {
        return chase_horizontal(p)% 2 == 1;
    }

    void check_validity() {
        //check_validity_aux(tree_);
    }

    void check_validity_aux(Node* p) {
        static int m = 0;
        m++;

        if (p == NULL) { return; }
        Leaf* leaf = p->as_leaf();
        if (leaf) {
            if (leaf->upperright) {
                assert(leaf->upperright->id()<1000);
            }
            assert(leaf->bottom <(Segment*)0xf0000000);
            assert(leaf->upperleft <(Leaf*)0xf0000000);
            assert(leaf->lowerleft <(Leaf*)0xf0000000);
            assert(leaf->upperright <(Leaf*)0xf0000000);
            assert(leaf->lowerright <(Leaf*)0xf0000000);
        } else {
            Node* car = p->car();
            Node* cdr = p->cdr();
            check_validity_aux(car);
            check_validity_aux(cdr);
        }
    }

#if 0
    bool search(Node* t, Node* q) {
        if (t == q) {
            return true;
        }

        bool f = false;
        if (t->car()) { f |= search(t->car(), q); }
        if (t->cdr()) { f |= search(t->cdr(), q); }
        return f;
    }
#endif

    void output_dot(std::string filename) {
        std::ofstream ofs(filename.c_str());
        ofs << "digraph { " << std::endl;
        ofs << "  node [shape = record];" << std::endl;
        std::set<Node*> mark;
        tree_->draw(mark, ofs);
        ofs << " }" << std::endl;
    }

    Leaf* new_leaf() {
        return link_node(new Leaf);
    }

    template <class T>
    T* link_node(T* p) {
        p->next(node_chain_);
        if (node_chain_) { node_chain_->prev(p); }
        node_chain_ = p;
        return p;
    }

    void delete_leaf(Leaf* p) {
        if (p->prev()) { p->prev()->next(p->next()); }
        if (p->next()) { p->next()->prev(p->prev()); }
        if (node_chain_ == p) { node_chain_ = p->next(); }
        delete p;
    }

    Segment* new_segment(const Point& p0, const Point& p1,
                         bool swapped, bool border,
                         const SegmentProperty& property) {
        Segment* p = new Segment(
            segment_id_seed_++, p0, p1, swapped, border, property);
        p->next(segment_chain_);
        if (segment_chain_) { segment_chain_->prev(p); }
        segment_chain_ = p;
        return p;
    }

private:
    Node*  tree_;
    Node*  node_chain_;
    Segment* segment_chain_;
    int   segment_id_seed_;

    int   lowest_score_;
    int   highest_score_;

};

template < class R, class SegmentProperty >
class TrapezoidalMapMachine {
public:
    typedef typename TrapezoidalMap<R, SegmentProperty>::Point Point;
    typedef typename TrapezoidalMap<R, SegmentProperty>::Segment Segment;

public:
    TrapezoidalMapMachine() {}
    TrapezoidalMapMachine(const TrapezoidalMap<R, SegmentProperty>& tm) {
        tm.compile(code_);
    }

    void init(const TrapezoidalMap<R, SegmentProperty>& tm) {
        code_.clear();
        tm.compile(code_);
    }

    bool find(
        const Point& q,
        Point& p0,
        Point& p1,
        Point& p2,
        Point& p3,
        int& score) const {
        SegmentProperty tsp, bsp;
        return find(q, p0, p1, p2, p3, score, tsp, bsp);
    }

    bool find(
        const Point& q,
        Point& p0,
        Point& p1,
        Point& p2,
        Point& p3,
        int& score,
        SegmentProperty& tsp,
        SegmentProperty& bsp) const {
        const char* b = &code_[0];
        const char* p = b + 4;

        switch (*((int*)p)) {
            case 0: return false;
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE1: {
            float x = *((float*)(p+4));
            float y = *((float*)(p+8));
            int offset;
            if (q.x()<x ||(q.x() == x && q.y()<y)) {
                offset = *((boost::uint32_t*)(p+12));
            } else {
                offset = *((boost::uint32_t*)(p+16));
            }
            if (offset == 0) { return false; }
            p = b + offset;
        }
        switch (*((int*)p)) {
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE2: {
            float p0x = *((float*)(p+4));
            float p0y = *((float*)(p+8));
            float p1x = *((float*)(p+12));
            float p1y = *((float*)(p+16));

            float y;
            if (p0x == p1x) {
                assert(p0x == q.x());
                y = p0y;
            } else {
                float la = *((float*)(p+20));
                float lb = *((float*)(p+24));
                y = calc_y(q.x(), p0x, p0y, p1x, p1y, la, lb);
            }

            int offset;
            if (q.y()<y) {
                offset = *((boost::uint32_t*)(p+28));
            } else {
                offset = *((boost::uint32_t*)(p+32));
            }
            if (offset == 0) { return false; }
            p = b + offset;
        }
        switch (*((int*)p)) {
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE3: {
            float tp0x = *((float*)(p+4));
            float tp0y = *((float*)(p+8));
            float tp1x = *((float*)(p+12));
            float tp1y = *((float*)(p+16));
            float tla = *((float*)(p+20));
            float tlb = *((float*)(p+24));
            float bp0x = *((float*)(p+28));
            float bp0y = *((float*)(p+32));
            float bp1x = *((float*)(p+36));
            float bp1y = *((float*)(p+40));
            float bla = *((float*)(p+44));
            float blb = *((float*)(p+48));
            float lx = *((float*)(p+52));
            float rx = *((float*)(p+56));

            p0.x(lx);
            p1.x(rx);
            p2.x(rx);
            p3.x(lx);
            p0.y(calc_y(lx, tp0x, tp0y, tp1x, tp1y, tla, tlb));
            p1.y(calc_y(rx, tp0x, tp0y, tp1x, tp1y, tla, tlb));
            p2.y(calc_y(rx, bp0x, bp0y, bp1x, bp1y, bla, blb));
            p3.y(calc_y(lx, bp0x, bp0y, bp1x, bp1y, bla, blb));
            score = *((boost::uint32_t*)(p+60));

            tsp = *((SegmentProperty*)(p+64));
            bsp = *((SegmentProperty*)(p+64+sizeof(SegmentProperty)));
            return true;
        }
    }


    bool find(
        const Point& q,
        int& score,
        SegmentProperty& top_segment_property,
        SegmentProperty& bottom_segment_property) const {
        const char* b = &code_[0];
        const char* p = b + 4;

        // computed goto version
        switch (*((int*)p)) {
            case 0: return false;
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE1: {
            float x = *((float*)(p+4));
            float y = *((float*)(p+8));
            int offset;
            if (q.x()<x ||(q.x() == x && q.y()<y)) {
                offset = *((boost::uint32_t*)(p+12));
            } else {
                offset = *((boost::uint32_t*)(p+16));
            }
            if (offset == 0) { return false; }
            p = b + offset;
        }
        switch (*((int*)p)) {
            case 0: return false;
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE2: {
            float p0x = *((float*)(p+4));
            float p0y = *((float*)(p+8));
            float p1x = *((float*)(p+12));
            float p1y = *((float*)(p+16));

            float y;
            if (p0x == p1x) {
                assert(p0x == q.x());
                y = p0y;
            } else {
                float la = *((float*)(p+20));
                float lb = *((float*)(p+24));
                y = calc_y(q.x(), p0x, p0y, p1x, p1y, la, lb);
            }

            int offset;
            if (q.y()<y) {
                offset = *((boost::uint32_t*)(p+28));
            } else {
                offset = *((boost::uint32_t*)(p+32));
            }
            if (offset == 0) { return false; }
            p = b + offset;
        }
        switch (*((int*)p)) {
            case 1: goto OPCODE1;
            case 2: goto OPCODE2;
            case 3: goto OPCODE3;
        }

      OPCODE3: {
            score = *((boost::uint32_t*)(p+60));
            top_segment_property = *((SegmentProperty*)(p+64));
            bottom_segment_property =
                *((SegmentProperty*)(p+64+sizeof(SegmentProperty)));
            return true;
        }
    }

    float calc_y(float qx,
                 float p0x, float p0y, float p1x, float p1y,
                 float la, float lb) const {
        if (qx == p0x) {
            return p0y;
        } else if (qx == p1x) { 
            return p1y; 
        } else { 
            return la * qx + lb;
        }
    }

    template <class OS>
    void generate(OS& os) {
        // header
        os << "template <class R, class SegmentProperty>" << std::endl;
        os << "class TrapezoidalCompiledMachine { " << std::endl;
        os << "public:" << std::endl;
        os << ind(1)<< "typedef typename "
           << "TrapezoidalMap<R, SegmentProperty>::Point Point;"
           << std::endl;
        os << ind(1)<< "typedef typename "
           << "TrapezoidalMap<R, SegmentProperty>::Segment Segment;"
           << std::endl;
        os << std::endl;

        os << "public:" << std::endl;
        os << ind(1)<< "TrapezoidalCompiledMachine() {}" << std::endl;
        os << ind(1)<< "~TrapezoidalCompiledMachine() {}" << std::endl;
        os << ind(1)<< "bool find(" << std::endl;
        os << ind(2)<< "const Point& q, " << std::endl;
        os << ind(2)<< "int& score, " << std::endl;
        os << ind(2)<< "SegmentProperty& tsp, " << std::endl;
        os << ind(2)<< "SegmentProperty& bsp "
            ") const" << std::endl;
        os << ind(1)<< " { " << std::endl;
        os << ind(2)<< "float sy;" << std::endl;

        // core code
        const char* b = &code_[0];
        const char* p = b + 4;
        const char* e = b + code_.size();
        while (p <e) {
            os << ind(1)<< " ADDR" <<(p - b)<< ":" << std::endl;

            switch (*((int*)p)) {
                case 0 : DebugBreak(); break;
                case 1:
                    os << ind(2)<< "if (q.x()<float(" << *((float*)(p+4))
                       << ")||" << std::endl;
                    os << ind(2)<< "(q.x() == float(" << *((float*)(p+4))
                       << ")&& q.y()<float(" << *((float*)(p+8))<< "))) { "
                       << std::endl;
                    os << ind(3)<< "goto ADDR" << *((boost::uint32_t*)(p+12))
                       << ";" << std::endl;
                    os << ind(2)<< " } else { " << std::endl;
                    os << ind(3)<< "goto ADDR" << *((boost::uint32_t*)(p+16))
                       << ";" << std::endl;
                    os << ind(2)<< " }" << std::endl << std::endl;

                    p += 20;
                    break;

                case 2: {
                    float p0x = *((float*)(p+4));
                    float p0y = *((float*)(p+8));
                    float p1x = *((float*)(p+12));
                    float p1y = *((float*)(p+16));

                    if (p0x == p1x) {
                        os << ind(2)<< "sy = float(" << p0y << ");"
                           << std::endl;
                    } else {
                        float la = *((float*)(p+20));
                        float lb = *((float*)(p+24));

                        os << ind(2)<< "if (q.x() == " << p0x
                           << ") { sy = float(" << p0y << "); }" << std::endl;
                        os << ind(2)<< " else if (q.x() == " << p1x
                           << ") { sy = float(" << p1y << "); }" << std::endl;
                        os << ind(2)<< " else { sy = float(" << la
                           << ")* q.x()+ float(" << lb << "); }" << std::endl;
                        os << std::endl;
                    }

                    os << ind(2)<< "if (q.y()<sy) { " << std::endl;
                    os << ind(3)<< "goto ADDR" << *((boost::uint32_t*)(p+28))
                       << ";" << std::endl;
                    os << ind(2)<< " } else { " << std::endl;
                    os << ind(3)<< "goto ADDR" << *((boost::uint32_t*)(p+32))
                       << ";" << std::endl;
                    os << ind(2)<< " }" << std::endl << std::endl;

                    p += 36;
                }

                    break;

                case 3:
                    os << ind(2)<< " { " << std::endl;
                    os << ind(3)<< "score = "
                       << *((boost::uint32_t*)(p+60))<< ";" << std::endl;
                    os << ind(3)<< "const unsigned char tspb[] = { ";
                    for (size_t i = 0 ; i <sizeof(SegmentProperty); i++) {
                        os << int(unsigned char(*(p+64+i)))<< ", ";
                    }
                    os << " };" << std::endl;
                    os << ind(3)<< "const unsigned char bspb[] = { ";
                    for (size_t i = 0 ; i <sizeof(SegmentProperty); i++) {
                        os << int(unsigned char(*(p+64+sizeof(SegmentProperty)+i)))
                           << ", ";
                    }
                    os << " };" << std::endl;
                    os << ind(3)<< "tsp = *((SegmentProperty*)tspb);"
                       << std::endl;
                    os << ind(3)<< "bsp = *((SegmentProperty*)bspb);"
                       << std::endl;
                    os << ind(3)<< "return true;" << std::endl;
                    os << ind(2)<< " }" << std::endl << std::endl;

                    p += 64 + sizeof(SegmentProperty)* 2;
                    break;
            }
        }

        // footer
        os << ind(1)<< " }" << std::endl;
        os << " };\n" << std::endl;
    }

    std::string ind(int x) {
        return std::string(4 * x, ' ');
    }

private:
    std::vector<char>  code_;

};

#endif // TRAPEZOIDAL_MAP_HPP_
