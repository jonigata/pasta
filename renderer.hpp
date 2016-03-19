// 2009/01/29 Naoyuki Hirayama

/*!
	@file	  renderer.hpp
	@brief	  <ŠT—v>

	<à–¾>
*/

#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "zw/d3dfvf.hpp"
#include "pathview.hpp"

class Renderer {
private:
	typedef zw::fvf::vertex< (D3DFVF_XYZRHW|D3DFVF_DIFFUSE) > vertex_type;

public:
	Renderer()
	{
		offset_ = D3DXVECTOR2( -128, -128 );
		tmp_offset_ = offset_;
		zoom_ = 1 * ZOOM_DIVISION;
	}
	~Renderer() {}

	void set_offset( const D3DXVECTOR2& p ) { offset_ = p; }
	void set_tmp_offset( const D3DXVECTOR2& p ) { tmp_offset_ = p; }

	D3DXVECTOR2 get_offset() { return offset_; }
	D3DXVECTOR2 get_tmp_offset() { return tmp_offset_; }

	float	get_zoom();
	void	set_zoom( float );
	void	add_delta_to_zoom( int );

	void render(
		LPDIRECT3DDEVICE9				device,
		const std::vector< Primitive >& primitives,
		int								step );

private:
	void draw_primitives(
		const std::vector< Primitive >& primitives,
		int								step,
		vertex_type*&					v0,
		vertex_type*&					v1 );
		
	D3DXVECTOR2 vec2( const float* a )
	{
		return D3DXVECTOR2( a[0], a[1] );
	}

	D3DXVECTOR4 vec4( const D3DXVECTOR2& v )
	{
		return D3DXVECTOR4( v.x, v.y, 0, 1 );
	}

	enum {
		ZOOM_DIVISION = 1000,
	};

	D3DXVECTOR2 offmag( const D3DXVECTOR2& v )
	{
		return v * ( float(zoom_) / ZOOM_DIVISION ) - tmp_offset_;
	}

private:
	D3DXVECTOR2				offset_;
	D3DXVECTOR2				tmp_offset_;
	int						zoom_;

};

#endif // RENDERER_HPP_
