//
//  wkb.h
//

#ifndef wkb_h
#define wkb_h

#include "container.h"
#include <sstream>

namespace ml {
    struct point_d {
	double x;
	double y;
    };

    inline std::ostream & operator << (std::ostream &s, point_d const &p) {
	return s << p.x << ", " << p.y;
    }

    inline double distance(point_d const &p1, point_d const &p2) {
	double dx = (p1.x-p2.x);
	double dy = (p1.y-p2.y);
	return sqrt(dx*dx+dy*dy);
    }

    struct rect_d {
	point_d min;
	point_d max;

	static rect_d void_rect() {
	    return rect_d(std::numeric_limits<T>::max(),std::numeric_limits<T>::max(),
			     -std::numeric_limits<T>::max(),-std::numeric_limits<T>::max());
	}

	bool empty() const {
	    return (*this == rect_d::void_rect() || (width()==0 && height()==0));
	}

	inline double height() const {
	    return fabs(max.y-min.y);
	}
	
	inline double width() const {
	    return fabs(max.x-min.x);
	}
	rect_d & extend(const rect_d &r) {
	    if(r.empty())
		return *this;
	    min.x = std::min(min.x,std::min(r.min.x,r.max.x));
	    min.y = std::min(min.y,std::min(r.min.y,r.max.y));
	    
	    max.x = std::max(max.x,std::max(r.min.x,r.max.x));
	    max.y = std::max(max.y,std::max(r.min.y,r.max.y));
	    return *this;
	}

	template< typename C >
	rect_d & bounds( C begin, C end) {
	    for(; begin != end; ++begin) {
		min.x = std::min(min.x,begin->x);
		min.y = std::min(min.y,begin->y);
		max.x = std::max(max.x,begin->x);
		max.y = std::max(max.y,begin->y);
	    }
	    return *this;
	}
    };

    inline std::ostream & operator << (std::ostream &s,const rect_d &r) {
	return s << '[' << r.min << ',' << r.max << ']';
    }

} // namespace ml



namespace ml {
	
	class path_storage;
	
	class wkb {
		char const * m_data;
		unsigned int const & m_size;
		
	public:
		enum type_e{
			wkb_point = 1,
			wkb_line_string = 2,
			wkb_polygon = 3,
			wkb_multi_point = 4,
			wkb_multi_line_string = 5,
			wkb_multi_polygon = 6,
			wkb_geometry_collection = 7
		};
		
		static char const *type_to_text(type_e type) {
			switch (type) {
				case wkb_point: return "wkb_point";
				case wkb_line_string: return "wkb_line_string";
				case wkb_polygon: return "wkb_polygon";
				case wkb_multi_point: return "wkb_multi_point";
				case wkb_multi_line_string: return "wkb_multi_line_string";
				case wkb_multi_polygon: return "wkb_multi_polygon";
				case wkb_geometry_collection: return "wkb_geometry_collection";
				default: return "unknown";
			}
		}
		
		typedef container<ml::point_d> line;
		
		wkb(char const *data) : m_data(data), m_size(*(unsigned int*)(m_data+5)) {}
		
		wkb & operator = (wkb const &d) { m_data = d.m_data; return *this; }
		
		inline char byte_order() const { return *m_data; }
		
		inline type_e type() const { return (type_e)*(int*)(m_data+1); }
		inline unsigned int size() const { return type() == wkb_point ? 1 : m_size; }
		
		line linestring() const {
			return line((line::value_type*)(m_data+9), size());
		}
		
		ml::point_d const & point() const {
			return *(ml::point_d*)(m_data+5);
		}
		
		wkb linestring(size_t index) const {
			assert(index>=0 && index<m_size);
			const char *ptr = m_data+9;
			while (index--) {
				ptr += 9 + (*(unsigned int*)(ptr+5) * 16);
			}
			return wkb(ptr);
		}
		
		wkb polygon(size_t index) const {
			assert(index>=0 && index<m_size);
			const char *ptr = m_data+9;
			while (index--) {
				ptr = end_of_polygon(ptr);
			}
			return wkb(ptr);
		}

		line ring(size_t index) const {
			assert(index>=0 && index<m_size);
			const char *ptr = m_data+9;
			while (index--) {
				ptr = end_of_ring(ptr);
			}
			return line((line::value_type*)(ptr+4),(line::value_type*)end_of_ring(ptr));
		}
		
		inline const char * end_of_ring(const char *ptr) const {
			return ptr + 4 + (*(unsigned int *)(ptr) * 16);
		}
		
		inline const char * end_of_polygon(const char *ptr) const {
			for (size_t sz = *(unsigned int*)((ptr+=9)-4);sz--;)
				ptr += (*(unsigned int *)(ptr) * 16) + 4;
			return ptr;
		}
		
		

		template< typename T >
		T & apply( T & func) const {
			switch (type()) {
				case wkb_polygon: {
					const char * ptr = m_data+9;
					for(size_t i=0; i<m_size; ++i) {
						ml::point_d *begin = (ml::point_d*)(ptr+4);
						ml::point_d *end = begin + *(unsigned int *)(ptr);
						ptr = (const char *)end;
						func(begin,end);
					}
				} break;
				case wkb_multi_polygon: {
					const char * ptr = m_data+9;
					for(size_t i=0; i<m_size; ++i) {
						for (size_t sz = *(unsigned int*)((ptr+=9)-4);sz--;) {
							ml::point_d *begin = (ml::point_d*)(ptr+4);
							ml::point_d *end = begin + *(unsigned int *)(ptr);
							ptr = (const char *)end;
							func(begin,end);
						}
					}
				} break;
				case wkb_point: {
					ml::point_d *begin = (ml::point_d*)(m_data+5);
					ml::point_d *end = begin + 1;
					func(begin,end);
				} break;
				case wkb_line_string: {
					ml::point_d *begin = (ml::point_d*)(m_data+9);
					ml::point_d *end = begin + size();
					func(begin,end);
				} break;
				case wkb_multi_line_string: {
					const char * ptr = m_data+9;
					for(size_t i=0; i<m_size; ++i) {
						ml::point_d *begin = (ml::point_d*)(ptr+9);
						ml::point_d *end = begin + *(unsigned int *)(ptr+5);
						ptr = (const char *)end;
						func(begin,end);
					}
				} break;
				default:
					break;
			}
			return func;
		}
		
		
		
		ml::rect_d bounds() const;
		std::string to_geo_json() const;
	};
	
	void make_wkb_point(std::vector<char> &wkb, ml::point_d const &pt);
} // namespace ml


#endif
