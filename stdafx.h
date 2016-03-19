/*!
  @file     stdafx.h
  @brief    <ŠT—v>

  <à–¾>
  $Id: stdafx.h 244 2007-05-02 08:57:27Z naoyuki $
*/
#ifndef STDAFX_H
#define STDAFX_H

#pragma warning(disable: 4996)
#pragma warning(disable: 4996)
#pragma warning(disable: 4099)
#pragma warning(disable: 4244)
#pragma warning(disable: 4396)
#pragma warning(disable: 4503)
#pragma warning(disable: 4800)
#pragma warning(disable: 4819)

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x400
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>
#include <deque>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>

#define __GMP_WITHIN_CONFIGURE
#include <CGAL/Point_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Quotient.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Segment_Delaunay_graph_filtered_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Bbox_2_Line_2_intersection.h>
#include <CGAL/Bbox_2_Ray_2_intersection.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Partition_traits_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_adaptation_traits_2.h> 
#include <CGAL/Segment_Delaunay_graph_adaptation_policies_2.h> 
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Boolean_set_operations_2.h>

#include "zw/window_manager.hpp"
#include "zw/basic_window.hpp"
#include "zw/d3d.hpp"
#include "zw/d3dfvf.hpp"
#include "zw/game_timer.hpp"

#include "zw/user/dashboard.hpp"
#include "zw/user/empty_user.hpp"
#include "zw/user/window_user.hpp"
#include "zw/user/dashboard_user.hpp"
#include "zw/user/d3d_user.hpp"
#include "zw/user/d3d_draw.hpp"

#endif // STDAFX_H
