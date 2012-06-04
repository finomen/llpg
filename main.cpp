/*
 * main.cpp
 *
 *  Created on: May 30, 2012
 *      Author: Nikolay Filchenko
 */

#include "llpg2.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>

using boost::lambda::_1;
using boost::lambda::_2;
using boost::lambda::_3;


int main() {

	rule<int> expression("expression"), more_terms("more_terms"),
			term("term"), more_factors("more_factors"), factor("factor"), number("number");
	rule<char> plus("plus"), minus("minus"), star("star"), lbrace("lbrace"), rbrace("rbrace");
	plus = terminal<char>("[+]");
	minus = terminal<char>("[-]");
	star = terminal<char>("[*]");
	lbrace = terminal<char>("[(]");
	rbrace = terminal<char>("[)]");
	number = terminal<int>("-?[0-9]*");


	expression = (term >> more_terms) [_1 + _2];
	more_terms = (plus >> term >> more_terms) [_2 + _3] | (minus >> term >> more_terms) [-_2 + _3];
	more_terms = epsilon<int>();
	term = (factor >> more_factors) [_1 * _2];
	more_factors = (star >> factor >> more_factors) [_2 * _3];
	more_factors = epsilon<int>(1);
	factor = (lbrace >> expression >> rbrace) [+_2] | number;


	std::vector<std::string> v;

	std::string s = "( 2 + 3 ) * 2 - 1";
	boost::split(v, s, boost::is_any_of(" "), boost::token_compress_on);

	std::vector<std::string>::iterator it1 = v.begin();
	std::cout << expression.parse(it1, v.end()) << std::endl;
	return 0;
}
