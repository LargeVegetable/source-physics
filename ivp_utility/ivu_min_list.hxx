// Copyright (C) Ipion Software GmbH 1999-2000. All rights reserved.

#ifndef IVP_U_MINLIST_INCLUDED
#define IVP_U_MINLIST_INCLUDED
// IVP_EXPORT_PUBLIC

typedef float IVP_U_MINLIST_FIXED_POINT;
#define IVP_U_MINLIST_MAXVALUE 1e10f

#define IVP_U_MINLIST_USELONG
typedef unsigned int IVP_U_MINLIST_INDEX;

#define IVP_U_MINLIST_UNUSED ( (1<<16) -1 )
#define IVP_U_MINLIST_LONG_UNUSED ( (1<<16) -2 )
#define IVP_U_MINLIST_MAX_ALLOCATION ( (1<<16) - 4 )


class IVP_U_Min_List_Element {
	public:
#ifdef IVP_U_MINLIST_USELONG
		unsigned short  long_next;
		unsigned short  long_prev;
#endif
		unsigned short next;
		unsigned short prev;
		IVP_U_MINLIST_FIXED_POINT value;
		void *element;
};

class IVP_U_Min_List {
    friend class IVP_U_Min_List_Enumerator;
    unsigned short	malloced_size;
    unsigned short	free_list;
    IVP_U_Min_List_Element *elems;
public:
    IVP_U_MINLIST_FIXED_POINT min_value;
#ifdef IVP_U_MINLIST_USELONG
    unsigned short	first_long;
#endif
    unsigned short	first_element;
    unsigned short	counter;
    
    IVP_U_Min_List(int size=4);
    ~IVP_U_Min_List();
    
    IVP_U_MINLIST_INDEX add(void *elem,IVP_U_MINLIST_FIXED_POINT value); // returns an index
    
    void *find_min_elem(){
		IVP_ASSERT( first_element != IVP_U_MINLIST_UNUSED);
		return elems[first_element].element;
	};

    IVP_BOOL has_elements(){
	return (IVP_BOOL) (counter>0);
    }

    IVP_U_MINLIST_FIXED_POINT find_min_value(){
		return min_value;
	}

    void prefetch0_minlist(){
        IVP_IF_PREFETCH_ENABLED(IVP_TRUE){
	    IVP_PREFETCH( this, 0 );
	}
    }

    void prefetch1_minlist(){
        IVP_IF_PREFETCH_ENABLED(IVP_TRUE){
	    IVP_PREFETCH_BLOCK( &elems[first_element], 64 ); // get at least 64 bytes
#ifdef IVP_U_MINLIST_USELONG
	    IVP_PREFETCH( &elems[first_long],0 );
#endif
	}
    }

    void remove_minlist_elem(IVP_U_MINLIST_INDEX index);
    void check();
#ifdef PLATFORM_VITA
    // Vita: whether elem is in the list at index. Free slots hold no element
    // (removal clears it), so the hull manager can refuse to remove a
    // listener twice or add it twice; either one corrupted the list
    IVP_BOOL holds(IVP_U_MINLIST_INDEX index, const void *elem) const {
	return (IVP_BOOL)( index < malloced_size && elems[index].element == elem );
    }
    IVP_BOOL index_in_range(IVP_U_MINLIST_INDEX index) const {
	return (IVP_BOOL)( index < malloced_size );
    }
    // For a list whose links are damaged: the elements its slots hold (a
    // removed slot holds none), then an empty list, consistent again
    unsigned int vita_slots() const { return malloced_size; }
    int vita_live_elements(void **out, int max) const {
	int n = 0;
	for ( unsigned int i = 0; i < malloced_size && n < max; i++ ) {
	    if ( elems[i].element ) out[n++] = elems[i].element;
	}
	return n;
    }
    void vita_reset_empty() {
	for ( unsigned int i = 0; i < malloced_size; i++ ) {
	    elems[i].next = i + 1;
	    elems[i].element = NULL;
	}
	elems[malloced_size - 1].next = IVP_U_MINLIST_UNUSED;
	free_list = 0;
	first_element = IVP_U_MINLIST_UNUSED;
#ifdef IVP_U_MINLIST_USELONG
	first_long = IVP_U_MINLIST_UNUSED;
#endif
	min_value = IVP_U_MINLIST_MAXVALUE;
	counter = 0;
    }
#endif
};

class IVP_U_Min_List_Enumerator {
    IVP_U_Min_List *min_list;
    IVP_U_MINLIST_INDEX loop_elem;
public:
    IVP_U_Min_List_Enumerator( IVP_U_Min_List *mh){
	min_list = mh;
	loop_elem = mh->first_element;
    }
    
    void *get_next_element(){
	void *e;
	if (loop_elem != IVP_U_MINLIST_UNUSED) {
	    e = min_list->elems[loop_elem].element;
	    loop_elem = min_list->elems[loop_elem].next;
	}else{
	    e = NULL;
	}
	return e;
    }

    IVP_U_Min_List_Element *get_next_element_header(){
	IVP_U_Min_List_Element *el; 
	if (loop_elem != IVP_U_MINLIST_UNUSED) {
	    el = &min_list->elems[loop_elem];
	    loop_elem = el->next;
	}else{
	    el = (IVP_U_Min_List_Element *)NULL;
	}
	return el;
    }

    
    void *get_next_element_lt(IVP_FLOAT max_limit){
	void *e;
	if (loop_elem != IVP_U_MINLIST_UNUSED) {
	    IVP_U_Min_List_Element *el = &min_list->elems[loop_elem]; 
	    if (el->value >= max_limit) return NULL;
	    loop_elem = min_list->elems[loop_elem].next;
	    e = el->element;
	}else{
	    e = NULL;
	}
	return e;
    }
};
#endif
