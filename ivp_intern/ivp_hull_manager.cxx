// Copyright (C) Ipion Software GmbH 1999-2000. All rights reserved.

#ifndef WIN32
#	pragma implementation "ivp_hull_manager_macros.hxx"
#	pragma implementation "ivp_listener_hull.hxx"
#	pragma implementation "ivp_hull_manager.hxx"
#endif

#include <ivp_physics.hxx>

#include <ivu_min_hash.hxx>
#include <ivp_hull_manager.hxx>
#include "ivp_hull_manager_macros.hxx"
#ifdef PLATFORM_VITA
#include <stdint.h>
#include <ivu_vita_finite.hxx>	// so_trace
extern "C" const char *so_module_for_address( uintptr_t addr, uintptr_t *offset );	// launcher

// The first few problems, with the code that ran into them (module+offset,
// for addr2line on the module), then every 1000th; the count says how often
static int s_nVitaHullProblems;
void ivp_vita_hull_problem( const char *what, IVP_Hull_Manager *hm, IVP_Listener_Hull *syn )
{
    const uintptr_t caller = (uintptr_t)__builtin_return_address( 0 );
    const int n = __atomic_add_fetch( &s_nVitaHullProblems, 1, __ATOMIC_RELAXED );
    if ( n > 8 && n % 1000 )
	return;
    uintptr_t offset = 0;
    const char *module = so_module_for_address( caller, &offset );
    so_trace( "[vita] ivp: hull listener %p %s (hull manager %p), from %s+0x%x; %d so far",
	(void *)syn, what, (void *)hm, module ? module : "?", (unsigned)( module ? offset : caller ), n );
}

// A list whose links are damaged (the chain reaches a free slot, runs past
// the array or loops) can't be walked or unlinked: removing a free slot
// writes through its stale links. The listeners still in it (slots that
// hold one) are told the hull manager is going, on an emptied list, so
// none keeps a pointer to it; their removals are refused, as they're no
// longer in it
static void ivp_vita_hull_recover( IVP_Hull_Manager *hm, IVP_U_Min_List *list )
{
    ivp_vita_hull_problem( "list damaged (a free slot, past its slots, or a loop) as the hull manager was deleted", hm, NULL );
    const int nSlots = list->vita_slots();
    void **live = (void **)p_malloc( sizeof( void * ) * nSlots );
    const int nLive = live ? list->vita_live_elements( live, nSlots ) : 0;
    list->vita_reset_empty();
    for ( int i = 0; i < nLive; i++ )
	( (IVP_Listener_Hull *)live[i] )->hull_manager_is_going_to_be_deleted_event( hm );
    list->vita_reset_empty();
    P_FREE( live );
}
#endif

// IVP_EXPORT_PRIVATE

IVP_Hull_Manager_Base::IVP_Hull_Manager_Base(): sorted_synapses(8){
    gradient = center_gradient = 0.0f;
    hull_value_last_vpsi = 0.0f;
    hull_center_value_last_vpsi = 0.0f;
    hull_value_next_psi = 0.0f;
    time_of_next_reset = 0;
}

IVP_Hull_Manager_Base::~IVP_Hull_Manager_Base(){
    IVP_ASSERT( ! sorted_synapses.has_elements() );
}

void IVP_Hull_Manager::delete_hull_manager(){
#ifdef PLATFORM_VITA
    // Follows the chain, not the count, and takes off a listener that didn't
    // remove itself, so a list damaged earlier can't read past its end or
    // loop here (a nonzero count with an empty chain crashed every level
    // change and reload in one player's Route Kanal)
    for ( int nGuard = 0; sorted_synapses.first_element != IVP_U_MINLIST_UNUSED; nGuard++ ){
	const IVP_U_MINLIST_INDEX first = sorted_synapses.first_element;
	IVP_Listener_Hull *syn = NULL;
	if ( sorted_synapses.index_in_range( first ) && nGuard <= IVP_U_MINLIST_MAX_ALLOCATION )
	    syn = (IVP_Listener_Hull *)sorted_synapses.find_min_elem();
	if ( !syn ){
	    ivp_vita_hull_recover( (IVP_Hull_Manager *)this, &sorted_synapses );
	    break;
	}
	syn->hull_manager_is_going_to_be_deleted_event((IVP_Hull_Manager *)this);
	if ( sorted_synapses.first_element == first && sorted_synapses.holds( first, syn ) ){
	    ivp_vita_hull_problem( "stayed in the list as the hull manager was deleted", (IVP_Hull_Manager *)this, syn );
	    sorted_synapses.remove_minlist_elem( first );
	}
    }
    if ( sorted_synapses.counter ){
	ivp_vita_hull_problem( "count left over as the hull manager was deleted", (IVP_Hull_Manager *)this, NULL );
	sorted_synapses.counter = 0;
    }
#else
    while ( sorted_synapses.has_elements() ){
	IVP_Listener_Hull *syn = (IVP_Listener_Hull *)sorted_synapses.find_min_elem();
	syn->hull_manager_is_going_to_be_deleted_event((IVP_Hull_Manager *)this);
    }
#endif
}

void   IVP_Listener_Hull::hull_manager_is_reset(IVP_FLOAT /*dt*/, IVP_FLOAT /*center_dt*/){
    ;
}

void IVP_Hull_Manager::reset_times(){
    IVP_FLOAT mt = - hull_value_last_vpsi;
    IVP_FLOAT mc = - hull_center_value_last_vpsi;
    IVP_U_Min_List_Enumerator mle(&sorted_synapses);
    
    while ( IVP_U_Min_List_Element *el = mle.get_next_element_header()){
	IVP_Listener_Hull *lh = (IVP_Listener_Hull *)el->element;
	el->value += mt;
	lh->hull_manager_is_reset(mt,mc);
	
    }
    sorted_synapses.min_value += mt;
    hull_value_last_vpsi = 0.0f;
    hull_center_value_last_vpsi = 0.0f;
    hull_value_next_psi += mt;
}

void IVP_Hull_Manager::reset_time(IVP_Time offset){
    last_vpsi_time -= offset;
}
