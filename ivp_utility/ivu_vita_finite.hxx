// Vita port: finiteness tests for the physics' safety nets (NaN and
// infinite core state, see ivp_calc_next_psi_solver.cxx). Integer tests on
// the exponent bits, like Source's IsFinite: the physics is built with
// -ffast-math, which lets the compiler fold isnan, isfinite and x != x
// away. GCC keeps integer masks, but clang folds even those under
// -ffinite-math-only; the empty asm hides the bits from the optimizer (no
// instruction).

#ifndef IVU_VITA_FINITE_INCLUDED
#define IVU_VITA_FINITE_INCLUDED

#ifdef PLATFORM_VITA

#include <string.h>

inline bool ivp_vita_finite( float f )
{
	unsigned int u;
	memcpy( &u, &f, sizeof( u ) );
	__asm__( "" : "+r"( u ) );
	return ( u & 0x7f800000u ) != 0x7f800000u;
}

inline bool ivp_vita_finite( double d )
{
	unsigned long long u;
	memcpy( &u, &d, sizeof( u ) );
	__asm__( "" : "+r"( u ) );
	return ( u & 0x7ff0000000000000ull ) != 0x7ff0000000000000ull;
}

inline bool ivp_vita_finite( const IVP_U_Float_Point &p )
{
	return ivp_vita_finite( p.k[0] ) && ivp_vita_finite( p.k[1] ) && ivp_vita_finite( p.k[2] );
}

#ifndef IVP_NO_DOUBLE
inline bool ivp_vita_finite( const IVP_U_Point &p )
{
	return ivp_vita_finite( p.k[0] ) && ivp_vita_finite( p.k[1] ) && ivp_vita_finite( p.k[2] );
}
#endif

inline bool ivp_vita_finite( const IVP_U_Quat &q )
{
	return ivp_vita_finite( q.x ) && ivp_vita_finite( q.y ) && ivp_vita_finite( q.z ) && ivp_vita_finite( q.w );
}

// Boot log (launcher_vita/so_loader.cpp)
extern "C" void so_trace( const char *fmt, ... );

#endif // PLATFORM_VITA

#endif // IVU_VITA_FINITE_INCLUDED
