//
//  wkb.cpp
//

#include "wkb.h"

void ml::make_wkb_point(std::vector<char> &wkb, ml::point_d const &pt) {
	wkb.resize(21);
	wkb[0] = 1;
	*((unsigned int *)&wkb[1]) = 1; // wkb_point
	*((double *)&wkb[5]) = pt.x;
	*((double *)&wkb[13]) = pt.y;
}

ml::rect_d ml::wkb::bounds() const {
	struct bounds_counter {
		ml::rect_d r;
		bounds_counter() : r(ml::rect_d::void_rect()) {}
		void operator()(ml::point_d const *begin, ml::point_d const *end) {
			r.bounds(begin,end);
		}
		ml::rect_d bounds() const {
			return r;
		}
	};
	bounds_counter b;
	return apply(b).bounds();
}


std::string ml::wkb::to_geo_json() const {
	struct point_formatter {
		std::stringstream &ss;
		unsigned int counter;
		bool parts;
		point_formatter(std::stringstream &s) : ss(s),counter(0), parts(false) {}
		void operator() (ml::point_d const *begin, ml::point_d const *end) {
			if (parts) ss << (counter ? ",[" : "[");
			ss << "[" << begin->x << "," << begin->y << "]";
			for(++begin;begin != end;++begin){
				ss << ",[" << begin->x << "," << begin->y << "]";
			}
			if (parts) ss << "]";
			++counter;
		}
		point_formatter & make_parts(bool p) {parts = p; counter = 0; return *this;}
	};
	
	std::stringstream ss;
	point_formatter fmt(ss);
	ss << std::fixed << std::showpoint << std::setprecision(6);
	ss << "{";
	switch(type()) {
		case wkb_point: {
			ss << "\"type\":\"Point\",\"coordinates\":";
			apply(fmt);
		} break;
			
		case wkb_line_string: {
			ss << "\"type\":\"LineString\",\"coordinates\": [";
			apply(fmt);
			ss << "]";
		} break;
			
		case wkb_multi_line_string: {
			ss << "\"type\":\"MultiLineString\",\"coordinates\": [";
			apply(fmt.make_parts(true));
			ss << "]";
		} break;
			
		case wkb_polygon: {
			ss << "\"type\":\"Polygon\",\"coordinates\": [";
			apply(fmt.make_parts(true));
			ss << "]";
		} break;
		case wkb_multi_polygon: {
			ss << "\"type\":\"MultiPolygon\",\"coordinates\": [";
			for(size_t i=0; i < size(); ++i) {
				ss << (i ? ",[" : "[");
				polygon(i).apply(fmt.make_parts(true));
				ss << "]";
			}
			ss << "]";
		} break;
		default: break;
	}
	ss << "}";
	return ss.str();
}

