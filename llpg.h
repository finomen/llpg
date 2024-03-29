/*
 * llpg.h
 *
 *  Created on: May 30, 2012
 *      Author: Nikolay Filchenko
 */

#ifndef LLPG_H_
#define LLPG_H_

#include <iostream>
#include <boost/preprocessor.hpp>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "tokenizer.h"

enum operation {
	SEQUENCE = 0,
	ALTERNATIVE = 1,
	ALIAS = 2,
	OPTIONAL = 3,
	LEAST_ONE = 4,
	MANY = 5,
	DELIMITED = 6,
	TERMINAL = 8
};

#define UNARY_OPERATIONS \
		((-, OPTIONAL))\
		((+, LEAST_ONE))\
		((*, MANY))

#define BINARY_OPERATIONS\
		((>>, SEQUENCE))\
		((|, ALTERNATIVE))\
		((%, DELIMITED))

class basic_rule_impl {
public:
	virtual ~basic_rule_impl() {}
	virtual void print() const = 0;
};

template<operation o>
struct mark;

template <typename Left, typename Right, operation Operation>
class rule_impl : public basic_rule_impl {
	template <typename L, typename R, operation Op>
	static void print_if_seq(rule_impl<L, R, Op> const & v) {
		v.print();
	}

	template <typename L, typename R>
	static void print_if_seq(rule_impl<L, R, SEQUENCE> const & v) {
		v.real_print((mark<SEQUENCE> *)0, false);
	}

	void real_print(const mark<SEQUENCE> * p, bool braces = true) const {
		if (braces)
			std::cout << "[";
		print_if_seq(data.left);
		std::cout << " ";
		print_if_seq(data.right);
		if (braces)
			std::cout << "]";
	}

	void real_print(const mark<ALTERNATIVE> *) const {
		std::cout << "{";
		data.left.print();
		std::cout << " | ";
		data.right.print();
		std::cout << "}";
	}

	void real_print(const mark<ALIAS> *) const {
		std::cout << data.left->name();
	}

	void real_print(const mark<TERMINAL> *) const {
		std::cout << data.left->to_string();
	}

	void real_print(const mark<OPTIONAL> *) const {
			std::cout << "-";
			data.left.print();
	}

	void real_print(const mark<LEAST_ONE> *) const {
			std::cout << "+";
			data.left.print();
	}

	void real_print(const mark<MANY> *) const {
			std::cout << "*";
			data.left.print();
	}
	void real_print(const mark<DELIMITED> *) const {
		data.left.print();
		std::cout << " % ";
		data.right.print();
	}
public:
	template<typename T>
	struct seqtype;
	template<typename T1, typename T2, operation o>
	struct seqtype <rule_impl<T1, T2, o> >{
		typedef rule_impl<rule_impl<Left, Right, Operation>, rule_impl<T1, T2, o>, SEQUENCE> result;
	};
	template<typename T>
	struct alttype;
	template<typename T1, typename T2, operation o>
	struct alttype <rule_impl<T1, T2, o> >{
		typedef rule_impl<rule_impl<Left, Right, Operation>, rule_impl<T1, T2, o>, ALTERNATIVE> result;
	};
	template<typename T, operation op>
	struct bintype;
	template<typename T1, typename T2, operation o, operation op>
	struct bintype <rule_impl<T1, T2, o>, op>{
		typedef rule_impl<rule_impl<Left, Right, Operation>, rule_impl<T1, T2, o>, op> result;
	};

	virtual ~rule_impl() {}


	virtual void print() const {
		real_print((mark<Operation>*)0);
	}

#define BINARY_OP(i, unused, elem)\
	template<typename T>\
	typename bintype<T, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(T const & r) const {\
		typename bintype<T, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result result;\
		result.data.left = *this;\
		result.data.right = r;\
		return result;\
	}
BOOST_PP_SEQ_FOR_EACH(BINARY_OP, ~, BINARY_OPERATIONS)
#undef BINARY_OP
#define UNARY_OP(r, unused, elem)\
	rule_impl<rule_impl<Left, Right, Operation>, void, BOOST_PP_TUPLE_ELEM(2, 1, elem)> operator BOOST_PP_TUPLE_ELEM(2, 0, elem)() {\
		rule_impl<rule_impl<Left, Right, Operation>, void, BOOST_PP_TUPLE_ELEM(2, 1, elem)> res;\
		res.data.left = *this;\
		return res;\
	}

BOOST_PP_SEQ_FOR_EACH(UNARY_OP, ~, UNARY_OPERATIONS)

#undef UNARY_OP

private:
	template<typename T1, typename T2, operation o>
	struct pair {
		T1 left;
		T2 right;
	};

	template<typename T1>
	struct pair<T1, void, ALIAS> {
		T1 * left;
	};
	template<typename T1>
	struct pair<T1, void, TERMINAL> {
		T1 * left;
	};
	template<typename T1>
	struct pair<T1, void, OPTIONAL> {
		T1 left;
	};

	template <typename T1, typename T2, operation o> friend class rule_impl;
public:
	pair<Left, Right, Operation> data;
};


template<typename T>
rule_impl<tokenclass<T>, void, TERMINAL> make_tokenclass(std::string const & name, std::string const & regex) {
	rule_impl<tokenclass<T>,void, TERMINAL> t;
	t.data.left = new tokenclass<T>(name, regex);
	return t;
}

#define TOKEN(T, name, regex) rule_impl<tokenclass<T>,void, TERMINAL> name = make_tokenclass<T>(#name, regex);

template<typename R>
class rule {
private:
public:
	typedef rule_impl<rule<R>, void, ALIAS> alias_impl;

	rule(std::string const & n = "unnamed-rule") : data(new pimpl) {
		data->name = n;
		data->impl = 0;
		data->rc = 1;
	}

	virtual ~rule() {
		if (!--data->rc) {
			delete data->impl;
			delete data;
		}
	}

	template <typename L, typename O, operation P>
	rule & operator =(rule_impl<L, O, P> const & value) {
		delete data->impl;
		data->impl = new rule_impl<L, O, P>(value);
		return *this;
	}

	void print() const {
		std::cout << data->name << " -> ";
		data->impl->print();
		std::cout << std::endl;
	}

	std::string const & name() const {
		return data->name;
	}

#define BINARY_OP(unused1, unused2, elem) \
	template <typename T>\
	typename alias_impl::template bintype<T, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(T const & v) {\
		alias_impl a;\
		a.data.left = this;\
		return a BOOST_PP_TUPLE_ELEM(2, 0, elem) v;\
	}\
\
	template <typename T>\
	typename alias_impl::template bintype<typename rule<T>::alias_impl, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(rule<T> & v) {\
		alias_impl a;\
		a.data.left = this;\
		typename rule<T>::alias_impl b;\
		b.data.left = &v;\
		return a BOOST_PP_TUPLE_ELEM(2, 0, elem) b;\
	}

BOOST_PP_SEQ_FOR_EACH(BINARY_OP, ~, BINARY_OPERATIONS)
#undef BINARY_OP
#define UNARY_OP(r, unused, elem)\
	rule_impl<alias_impl, void, BOOST_PP_TUPLE_ELEM(2, 1, elem)> operator BOOST_PP_TUPLE_ELEM(2, 0, elem)() {\
		rule_impl<alias_impl, void, BOOST_PP_TUPLE_ELEM(2, 1, elem)> res;\
		alias_impl a;\
		a.data.left = this;\
		res.data.left = a;\
		return res;\
	}
BOOST_PP_SEQ_FOR_EACH(UNARY_OP, ~, UNARY_OPERATIONS)
#undef UNARY_OP

private:
	struct pimpl {
		std::string name;
		basic_rule_impl * impl;
		size_t rc;
	} * data;

	rule(rule const & r) : data(r.data){
		++data->rc;
	}

	rule & operator=(rule const & r) {
		data = r.data;
		++data->rc;
		return *this;
	}

	template <typename T> friend class rule;

#define BINARY_OP(unused1, unused2, elem)\
	template <typename T, typename V>\
	friend typename T::template bintype<typename rule<V>::alias_impl, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(T const & v, rule<V> & u);\
	template <typename T, typename V>\
	friend typename rule<T>::alias_impl::template bintype<typename rule<V>::alias_impl, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(rule<T> const &v, rule<V> const &u);

BOOST_PP_SEQ_FOR_EACH(BINARY_OP, ~, BINARY_OPERATIONS)
#undef BINARY_OP
};

#define BINARY_OP(unused1, unused2, elem)\
template <typename T, typename V>\
typename T::template bintype<typename rule<V>::alias_impl, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(T const & v, rule<V> & u) {\
	typename rule<V>::alias_impl b;\
	b.data.left = &u;\
	return v BOOST_PP_TUPLE_ELEM(2, 0, elem) b;\
}\
\
template <typename T, typename V>\
typename rule<T>::alias_impl::template bintype<typename rule<V>::alias_impl, BOOST_PP_TUPLE_ELEM(2, 1, elem)>::result operator BOOST_PP_TUPLE_ELEM(2, 0, elem)(rule<T> const &v, rule<V> const &u) {\
	typename rule<T>::alias_impl a;\
	a.data.left = &v;\
	typename rule<T>::alias_impl b;\
	b.data.left = &u;\
	return a BOOST_PP_TUPLE_ELEM(2, 0, elem) b;\
}


BOOST_PP_SEQ_FOR_EACH(BINARY_OP, ~, BINARY_OPERATIONS)
#undef BINARY_OP

#undef UNARY_OPERATIONS
#undef BINARY_OPERATIONS



#endif /* LLPG_H_ */
