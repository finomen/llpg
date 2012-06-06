/*
 * main.cpp
 *
 *  Created on: May 30, 2012
 *      Author: Nikolay Filchenko
 */

#include "llpg2.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>


#include <boost/function.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/scope/lambda.hpp>
#include <boost/phoenix/fusion.hpp>
#include <algorithm>

using boost::phoenix::arg_names::_1;
using boost::phoenix::arg_names::_2;
using boost::phoenix::arg_names::_3;
using boost::phoenix::arg_names::_4;
using boost::phoenix::arg_names::_5;
using boost::phoenix::arg_names::_6;
using boost::phoenix::arg_names::_7;
using boost::phoenix::arg_names::_8;
using boost::phoenix::arg_names::_9;
using boost::phoenix::arg_names::_10;
using boost::phoenix::local_names::_a;
using boost::phoenix::local_names::_b;
using boost::phoenix::local_names::_c;
using boost::phoenix::local_names::_d;
using boost::phoenix::local_names::_e;
using boost::phoenix::local_names::_b;
using boost::phoenix::lambda;
using boost::phoenix::bind;

boost::function<int(int)> bind(int b, boost::function<int(int,int)> const & f) {
	return bind(f, _1, b);
}

boost::function<int(int)> bindm(int a, boost::function<int(int)> const & f) {
	return bind(f, _1 * a);
}

boost::function<int(int)> bindd(int a, boost::function<int(int)> const & f) {
	return bind(f, _1 / a);
}


int main() {

	rule<int> expression("expression"), more_terms("more_terms"),
			term("term"), factor("factor"), number("number");
	rule<char> plus("plus"), minus("minus"), star("star"), slash("slash"), lbrace("lbrace"), rbrace("rbrace");

	rule<boost::function<int(int)> > more_factors("more_factors");

	plus = terminal<char>("[+]");
	minus = terminal<char>("[-]");
	star = terminal<char>("[*]");
	slash = terminal<char>("[/]");
	lbrace = terminal<char>("[(]");
	rbrace = terminal<char>("[)]");
	number = terminal<int>("-?[0-9]*");

	expression = (term >> more_terms) [_1 + _2];
	more_terms = (plus >> term >> more_terms) [_2 + _3] | (minus >> term >> more_terms) [-_2 + _3];
	more_terms = epsilon<int>();
	term = (factor >> more_factors) [bind(_2, _1)];
	more_factors = (star >> factor >> more_factors) [bind(&bindm, _2, _3)];
	more_factors = (slash >> factor >> more_factors) [bind(&bindd, _2, _3)];
	more_factors = epsilon<boost::function<int(int)> >(+_1);
	factor = (lbrace >> expression >> rbrace) [+_2] | number;

	std::string s;
	std::vector<std::string> v;
	for(;;) {
		std::getline(std::cin, s);
		if (s.empty())
			break;
		v.clear();
		boost::split(v, s, boost::is_any_of(" "), boost::token_compress_on);
		std::vector<std::string>::iterator it1 = v.begin();
		try {
			int r =  expression.parse(it1, v.end());
            if (it1 == v.end()) {
                std::cout << s << " = " << r << std::endl;
            } else {
                std::cout << "EXPECTED EOF FOUND " << *it1 << std::endl;
            }
		} catch(parse_error const & e) {
            if (it1 == v.end()) {
                std::cerr << "UEXPECTED END OF LINE" << std::endl;
            } else {
    			std::cerr << "EXPECTED {" << e.what() << "} GOT " << *it1 << std::endl;
            }
		}
	}


	return 0;
}
