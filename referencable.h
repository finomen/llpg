#ifndef _H_REFERENCABLE_
#define _H_REFERENCABLE_

class referencable {
public:
    referencable() : refcount(0) {}
    virtual ~referencable() {};
private:
    size_t refcount;
    friend void intrusive_ptr_add_ref(referencable * p);
    friend void intrusive_ptr_release(referencable * p);
};

inline void intrusive_ptr_add_ref(referencable * p)
{
    ++(p->refcount);
}

inline void intrusive_ptr_release(referencable * p)
{
    if (--(p->refcount) == 0) {
        delete p;
    }
} 

#endif