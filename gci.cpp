// 2016/03/19 Naoyuki Hirayama

#include "gci.hpp"

namespace gci {

void read_gci(const char* filename, Document& doc) {
    std::ifstream ifs(filename);

    // 頂点リスト
    int vertex_count;
    ifs >> vertex_count;
    for (int i = 0 ; i <vertex_count ; i++) {
        int id;
        float x, y;
        ifs >> id >> x >> y;
        doc.vertices.push_back(D3DXVECTOR2(x, y));
    }

    // 入力ポリゴン
    int input_polygon_count;
    ifs >> input_polygon_count;
    for (int i = 0 ; i <input_polygon_count ; i++) {
        int id, c;
        ifs >> id >> c;
        std::vector<int> p;
        for (int j = 0 ; j <c ; j++) {
            int index;
            ifs >> index;
            p.push_back(index);
        }
        doc.input_polygons.push_back(p);
    }

    // 入力サイト
    int input_site_count;
    ifs >> input_site_count;
    for (int i = 0 ; i <input_site_count ; i++) {
        int id, type;
        ifs >> id >> type;
        Document::Site s;
        if (type == 1) {
            s.is_segment = false;
            ifs >> s.p0;
        } else {
            assert(type == 2);
            s.is_segment = true;
            ifs >> s.p0 >> s.p1;
        }
        doc.sites.push_back(s);
    }

    // ボロノイセル・三角形分割
    int voronoi_cell_count;
    ifs >> voronoi_cell_count;
    for (int i = 0 ; i <voronoi_cell_count ; i++) {
        Document::Cell cell;
        int id;
        ifs >> id >> cell.site_index;

        int c;
        ifs >> c;
        for (int j = 0 ; j <c ; j++) {
            int index;
            ifs >> index;
            cell.vertex_indices.push_back(index);
        }

        ifs >> c;
        for (int j = 0 ; j <c ; j++) {
            Document::Triangle t;
            ifs >> t.v0 >> t.v1 >> t.v2;
            cell.triangles.push_back(t);
        }

        doc.voronoi_cells.push_back(cell);
    }

    dprintf("");
}

} // namespace gci
