/*
 * main.cpp
 *
 *  Created on: May 30, 2012
 *      Author: Nikolay Filchenko
 */

#include "llpg.h"

#include <boost/fusion/container.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <algorithm>

int main() {
	rule<int> r1("r1");
	rule<int> r2("r2");
	rule<int> r3("r3");
	rule<int> r4("r4");
	rule<int> r5("r5");
	rule<int> r6("r6");
	rule<int> r7("r7");
	r1 = (r2 >> r3) | (r4 >> r2);
	r2 = r2 >> (r4 >> r2);
	r3 = r2 | (r4 >> r2);
	r4 = (r4 >> r2) | r2;
	r5 = (r4 >> r2) >> r2;
	r6 = -(r1 >> r2);
	r7 = -r6 % r5;
	r1.print();
	r2.print();
	r3.print();
	r4.print();
	r5.print();
	r6.print();
	r7.print();

	return 0;
}
