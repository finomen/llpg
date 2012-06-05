/*
 * llpg2.h
 *
 *  Created on: Jun 4, 2012
 *      Author: Nikolay Filchenko
 */

#ifndef LLPG2_H_
#define LLPG2_H_

#define MAX_LEN 20
#define FUSION_MAX_VECTOR_SIZE MAX_LEN
#include <string>
#include <boost/preprocessor.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm/transformation/join.hpp>
#include <boost/fusion/include/join.hpp>
#include <boost/function.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <vector>
#include <boost/regex.hpp>
#include <exception>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "referencable.h"
#include <boost/intrusive_ptr.hpp>

typedef std::vector<std::string>::iterator Iterator;

class unexpected_end_of_line : public std::exception{
};

class parse_error : public std::exception {

};

template<typename R>
class parsing_primitive : public referencable {
public:
	virtual ~parsing_primitive(){};
	virtual bool match(std::string const &) const = 0;
	virtual bool is_epsilon() const {return 0;}
	virtual R parse(Iterator & pos, Iterator const & end) const = 0;
};

template<typename R>
class binded_sequence : public parsing_primitive<R> {
public:
	template<typename Callback, typename Sequence>
	binded_sequence(Callback const & callable, Sequence const & seq) {
		data<Callback, Sequence> * np = new data<Callback, Sequence>(seq, callable);
		pimpl = np;
		pimpl->rc = 1;
	}

	~binded_sequence() {
		if (!--pimpl->rc) {
			delete pimpl;
		}
	}

	bool match(std::string const & s) const{
		return pimpl->match(s);
	}

	R parse(Iterator & pos, Iterator const & end) const {
		R res = pimpl->parse(pos, end);
		return res;
	}

	binded_sequence(binded_sequence const & o) {
		pimpl = o.pimpl;
		++pimpl->rc;
	}

private:
	struct adata {
		virtual ~adata(){}
		virtual R parse(Iterator & pos, Iterator const & end) = 0;
		virtual bool match(std::string const & s) const = 0;
		size_t rc;
	};
	template<typename Callback, typename Sequence>
	struct data : public adata{
		Sequence seq;
		Callback callable;
		data(Sequence const & s, Callback c) : seq(s), callable(c) {}
		virtual ~data(){}
		virtual R parse(Iterator & pos, Iterator const & end) {
			return boost::fusion::invoke(callable, seq.parse(pos, end));
		}

		virtual bool match(std::string const & s) const {
			return seq.match(s);
		}

	};
	adata * pimpl;
};


#define DTARG(n, i, data) , typename BOOST_PP_CAT(data, i)=void
#define TARG(n, i, data) , typename BOOST_PP_CAT(data, i)
#define TNARG(n, i, data) , BOOST_PP_CAT(data, i)
#define RARG(n, i, data) , rule<BOOST_PP_CAT(data, i)>
#define MATCH(n, i, data) if (boost::fusion::at_c<i>(rules).match(s)) return true; else if (!boost::fusion::at_c<i>(rules).is_epsilon()) return false;
#define PARSE(n, i, data) boost::fusion::at_c<i>(ans) = boost::fusion::at_c<i>(rules).parse(pos, end);
#define PRINT(n, i, data) << boost::fusion::at_c<i>(ans) << " "

template<size_t N BOOST_PP_REPEAT_FROM_TO(0, MAX_LEN, DTARG, T_)>
class sequence;

#define SEQ(count, N, data)\
template<typename T_0 BOOST_PP_REPEAT_FROM_TO(1, N, TARG, T_)>\
class sequence<N BOOST_PP_REPEAT_FROM_TO(0, N, TNARG, T_)>\
{\
public:\
	sequence(boost::fusion::vector<rule<T_0> BOOST_PP_REPEAT_FROM_TO(1, N, RARG, T_)> const & r) : rules(r){}\
	template<typename T>\
	sequence<BOOST_PP_INC(N) BOOST_PP_REPEAT_FROM_TO(0, N, TNARG, T_), T> operator >>(rule<T> const & t) {\
		sequence<BOOST_PP_INC(N) BOOST_PP_REPEAT_FROM_TO(0, N, TNARG, T_), T> res(\
		boost::fusion::join(rules, boost::fusion::make_vector(t)));\
		return res;\
	}\
	boost::fusion::vector<rule<T_0> BOOST_PP_REPEAT_FROM_TO(1, N, RARG, T_)> const & get_rules() const {\
		return rules;\
	}\
	boost::fusion::vector<T_0 BOOST_PP_REPEAT_FROM_TO(1, N, TNARG, T_)>  parse(Iterator & pos, Iterator const & end) const {\
		boost::fusion::vector<T_0 BOOST_PP_REPEAT_FROM_TO(1, N, TNARG, T_)> ans;\
		BOOST_PP_REPEAT_FROM_TO(0, N, PARSE, ~)\
		return ans;\
	}\
	template<typename C>\
	boost::intrusive_ptr<binded_sequence<typename boost::fusion::result_of::invoke<C, boost::fusion::vector<T_0 BOOST_PP_REPEAT_FROM_TO(1, N, TNARG, T_)> >::type> > operator[](C const & callback) {\
		return new binded_sequence<typename boost::fusion::result_of::invoke<C, boost::fusion::vector<T_0 BOOST_PP_REPEAT_FROM_TO(1, N, TNARG, T_)> >::type>(callback, (*this));\
	}\
	bool match(std::string const & s) const {\
		BOOST_PP_REPEAT_FROM_TO(0, N, MATCH, ~)\
		return false;\
	}\
private:\
	boost::fusion::vector<rule<T_0> BOOST_PP_REPEAT_FROM_TO(1, N, RARG, T_)> rules;\
	template<size_t n BOOST_PP_REPEAT_FROM_TO(0, MAX_LEN, TARG, T__)> friend class sequence;\
};

template<typename T>
class alternative {
public:
	alternative(boost::intrusive_ptr<binded_sequence<T> > const & seq) {
		data.push_back(seq);
	}
	alternative operator |(boost::intrusive_ptr<binded_sequence<T> > const & s) {
		alternative res = (*this);
		res.data.push_back(s);
		return res;
	}

	std::vector<boost::intrusive_ptr<binded_sequence<T> > > const & get() const {
		return data;
	}
private:
	std::vector<boost::intrusive_ptr<binded_sequence<T> > > data;
};

template<typename T>
alternative<T> operator |(boost::intrusive_ptr<binded_sequence<T> > const & a, boost::intrusive_ptr<binded_sequence<T> > const & b) {
	return alternative<T>(a) | b;
}

template<typename T, typename T1>
alternative<T> operator |(boost::intrusive_ptr<binded_sequence<T> > const & a, T1 const & b) {
	return alternative<T>(a) | b;
}

template<typename T>
T id(T const & a) {
	return a;
}

template<typename T>
class terminal : public parsing_primitive<T> {
public:
	terminal(std::string const & regex) : e(regex) {
	}

	bool match(std::string const & s) const {
		return regex_match(s, e);
	}

	T parse(Iterator & pos, Iterator const & end) const {
		if (!match(*pos)) {
			throw parse_error();
		}
		T r =  boost::lexical_cast<T>(*(pos++));
		//std::cout << ">>>> TERMINAL " << r << std::endl;
		return r;
	}

private:
	boost::regex e;
};

template<typename T>
struct epsilon {
T value;
epsilon(T const & v = T()) : value(v) {};
};

template<typename T>
class rule {
public:
	rule(std::string const & name = "unnamed-rule") : is_epsilon_(boost::make_shared<bool>(0)),
		seq(boost::make_shared<std::vector<boost::intrusive_ptr<parsing_primitive<T> > > >()),
		name(boost::make_shared<std::string>(name)),
		default_value(boost::make_shared<T>()){}

	void operator =(epsilon<T> const & e) {
		*is_epsilon_ = true;
		*default_value = e.value;
	}

	void operator =(boost::intrusive_ptr<binded_sequence<T> > const & t) {
		seq->push_back(t);
	}

	void operator =(terminal<T> const & t) {
		seq->push_back(new terminal<T>(t));
	}

	void operator =(alternative<T> const & t) {
		BOOST_FOREACH(boost::intrusive_ptr<binded_sequence<T> > const & s, t.get()) {
			seq->push_back(s);
		}
	}

	operator boost::intrusive_ptr<binded_sequence<T> >() const {
		return new binded_sequence<T>(&id<T>, (sequence<1, T>)(*this));
	}

	template<typename T1>
	sequence<2, T, T1> operator >>(rule<T1> const & t) {
		return (sequence<1, T>)(*this) >> t;
	}

	alternative<T> operator |(rule const & t) {
		return alternative<T>((boost::intrusive_ptr<binded_sequence<T> >)(*this)) | t;
	}

	bool is_epsilon() const {
		return *is_epsilon_;
	}

	bool match(std::string const & s) const{
		BOOST_FOREACH(boost::intrusive_ptr<parsing_primitive<T> > const & t, *seq) {
			if (t->match(s)) {
				return true;
			}
		}

		return is_epsilon();
	}

	T parse(Iterator & pos, Iterator const & end) const {
		if (pos == end) {
			if (!is_epsilon()) {
				throw parse_error();
			} else {
				return *default_value;
			}
		}

		if (!match(*pos)) {
			throw parse_error();
		}

		BOOST_FOREACH(boost::intrusive_ptr<parsing_primitive<T> > const & t, *seq) {
			if (t->match(*pos)) {
				T res = t->parse(pos, end);
				return res;
			}
		}
		assert(is_epsilon() != 0);
		return *default_value;
	}
private:
	boost::shared_ptr<bool> is_epsilon_;
	boost::shared_ptr<std::vector<boost::intrusive_ptr<parsing_primitive<T> > > > seq;
	boost::shared_ptr<std::string> name;
	boost::shared_ptr<T> default_value;

};



BOOST_PP_REPEAT_FROM_TO(1, MAX_LEN, SEQ, ~)

#endif /* LLPG2_H_ */
