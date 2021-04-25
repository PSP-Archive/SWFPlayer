/***********************************************************************************

  Module :	CVector.cpp

  Description :

  Last Modified $Date: $

  $Revision: $

  Copyright (C) 2006 - PSPHacks.net Development Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  Contact information
  PSPHacks.net Development Team <webmaster@psphacks.net>
  71M - 71M@Orange.net

***********************************************************************************/

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CVector.h"
#include "CMath.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************

//**********************************************************************************
//
//   V2 class implementation
//
//**********************************************************************************
V2::V2()
{
}

V2::V2( const V2 & src ) : x( src.x ), y( src.y )
{
}

V2::V2( const float ix, const float iy ) : x( ix ), y( iy )
{
}

V2::V2( const V2 * const p_src ) : x( p_src->x ), y( p_src->y )
{
}

//*************************************************************************************
//
//*************************************************************************************
V2 & V2::operator = (const V2 & vec)
{
	x = vec.x;
	y = vec.y;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V2 V2::operator + (const V2 & rhs) const
{
	return V2( x + rhs.x, y + rhs.y );
}

V2 & V2::operator += (const V2 & vector)
{
	x += vector.x;
	y += vector.y;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V2 V2::operator - (const V2 & rhs) const
{
	return V2 (x - rhs.x, y - rhs.y);
}

V2 & V2::operator -= (const V2 & vector)
{
	x += vector.x;
	y += vector.y;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V2 V2::operator * (const float rhs) const
{
	return V2 (x * rhs, y * rhs);
}

V2 & V2::operator *= (const float fScalar)
{
	x *= fScalar;
	y *= fScalar;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
V2 & V2::operator /= (const float fScalar)
{
	if (fScalar != 0.f)
	{
		*this *= (1.f / fScalar);
	}
	else
	{
		x = FLT_MAX;
		y = FLT_MAX;
	}

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
float V2::Length() const
{
	return SQRTF( LengthSq() );
}

float V2::LengthSq() const
{
	return x*x + y*y;
}

//*************************************************************************************
//
//*************************************************************************************
float V2::Dot( const V2 & vector ) const
{
	return x*vector.x + y*vector.y;
}

//*************************************************************************************
//
//*************************************************************************************
float V2::Angle() const
{
	return atan2f( x, y );
}

//*************************************************************************************
//
//*************************************************************************************
float V2::Normalise( const float tolerance )
{
	float length( Length() );

	if (length > FLT_MIN)
	{
		*this *= ( 1.f / length );
	}
	else
	{
		length = 0.f;
	}

	return length;
}

//*************************************************************************************
//
//*************************************************************************************
V2 V2::Lerp( const V2 & vector, const float weight ) const
{
	return (*this + (weight * (vector - *this)));
}

//*************************************************************************************
//
//*************************************************************************************
float V2::Distance( const V2 & vector ) const
{
	V2 delta( *this - vector );
	return delta.Length();
}

float V2::DistanceSq( const V2 & vector ) const
{
	V2 delta( *this - vector );
	return delta.LengthSq();
}

//*********************************************************************************
//	Vector2 Operators
//*********************************************************************************
inline V2 operator * (const float fScalar, const V2 &vec)
{
	return V2( fScalar * vec.x, fScalar * vec.y );
}

inline V2 operator + (const V2 &vec, const float &val)
{
	return V2( vec.x + val, vec.y + val );
}

inline V2 operator - (const V2 &vec)
{
	return V2( -vec.x, -vec.y );
}

inline V2 operator - (const V2 &vec, const float &val)
{
	return V2( vec.x - val, vec.y - val );
}


//*************************************************************************************
//
//	V3 class implementation
//
//*************************************************************************************

//*************************************************************************************
//
//*************************************************************************************
V3::V3()
{
}

V3::V3( const V3 & src ) : x( src.x ), y( src.y ), z( src.z )
{
}

V3::V3( const V3 * const p_src ) : x( p_src->x ), y( p_src->y ), z( p_src->z )
{
}

V3::V3( const float ix, const float iy, const float iz ) : x( ix ), y( iy ), z( iz )
{
}

//*************************************************************************************
//
//*************************************************************************************
V3::operator float * ()
{
	return &x;
}

V3::operator const float * () const
{
	return &x;
}

//*************************************************************************************
//
//*************************************************************************************
V3	V3::operator + () const
{
	return V3( +x, +y, +z );
}

V3	V3::operator - () const
{
	return V3( -x, -y, -z );
}

//*************************************************************************************
//
//*************************************************************************************
V3 & V3::operator = (const V3 & vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V3 V3::operator + (const V3 & rhs) const
{
	return V3 (x + rhs.x, y + rhs.y, z + rhs.z);
}

V3 & V3::operator += (const V3 & vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;

	return *this;
}

V3 & V3::operator += (const float & val)
{
	x += val;
	y += val;
	z += val;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V3 V3::operator - (const V3 & rhs) const
{
	return V3 (x - rhs.x, y - rhs.y, z - rhs.z);
}

V3 & V3::operator -= (const V3 & vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;

	return *this;
}

V3 & V3::operator -= (const float & val)
{
	x -= val;
	y -= val;
	z -= val;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V3 V3::operator * (const float rhs) const
{
	return V3 (x * rhs, y * rhs, z * rhs);
}

V3 & V3::operator *= (const float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
V3 & V3::operator /= (const float fScalar)
{
	if (fScalar != 0.f)
	{
		*this *= (1.f / fScalar);
	}
	else
	{
		x = FLT_MAX;
		y = FLT_MAX;
		z = FLT_MAX;
	}

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
bool	V3::operator == ( const V3 & rhs ) const
{
	return x == rhs.x && y == rhs.y && z == rhs.z;
}

bool	V3::operator != ( const V3 & rhs ) const
{
	return x != rhs.x && y != rhs.y && z != rhs.z;
}

bool V3::operator > ( const V3 & rhs ) const
{
	if ( x <= rhs.x )
		return false;

	if ( y <= rhs.y )
		return false;

	if ( z <= rhs.z )
		return false;

	return true;
}

bool V3::operator < ( const V3 & rhs ) const
{
	if ( x >= rhs.x )
		return false;

	if ( y >= rhs.y )
		return false;

	if ( z >= rhs.z )
		return false;

	return true;
}

bool V3::operator >= ( const V3 & rhs ) const
{
	if ( x < rhs.x )
		return false;

	if ( y < rhs.y )
		return false;

	if ( z < rhs.z )
		return false;

	return true;
}

bool V3::operator <= ( const V3 & rhs ) const
{
	if ( x > rhs.x )
		return false;

	if ( y > rhs.y )
		return false;

	if ( z > rhs.z )
		return false;

	return true;
}

//*************************************************************************************
//
//*************************************************************************************
float V3::Length() const
{
	return SQRTF( LengthSq() );
}

float V3::LengthSq() const
{
	return x*x + y*y + z*z;
}

//*************************************************************************************
//
//*************************************************************************************
float V3::Distance( const V3 & vector ) const
{
	V3 delta( *this - vector );
	return delta.Length();
}

float V3::DistanceSq( const V3 & vector ) const
{
	V3 delta( *this - vector );
	return delta.LengthSq();
}

//*************************************************************************************
//
//*************************************************************************************
float V3::HorizontalDistance() const
{
	return SQRTF( SQUARE( x ) + SQUARE( z ) );
}

float V3::HorizontalDistanceSq() const
{
	return SQUARE( x ) + SQUARE( z );
}

float V3::HorizontalDistance( const V3 & vector ) const
{
	return SQRTF( SQUARE( x - vector.x ) + SQUARE( z - vector.z ) );
}

float V3::HorizontalDistanceSq( const V3 & vector ) const
{
	return SQUARE( x - vector.x ) + SQUARE( z - vector.z );
}

//*************************************************************************************
//
//*************************************************************************************
float V3::Dot( const V3 & vector ) const
{
	return (x * vector.x + y * vector.y + z * vector.z);
}

//*************************************************************************************
//
//*************************************************************************************
float V3::Angle() const
{
	return atan2f( x, -z );
}

float V3::Pitch() const
{
	return atan2f( y, HorizontalDistance() );
}

//*************************************************************************************
//
//*************************************************************************************
bool V3::IsNormalised() const
{
	return FABSF( LengthSq() - 1.0f ) < FLT_MIN;
}

//*************************************************************************************
//
//*************************************************************************************
float V3::Normalise()
{
	float length( Length() );

	if (length > FLT_MIN)
	{
		*this *= ( 1.f / length );
	}
	else
	{
		length = 0.f;
	}

	return length;
}

V3 V3::Normal() const
{
	V3	result( *this );

	result.Normalise();

	return result;
}

//*************************************************************************************
//
//*************************************************************************************
V3 V3::Cross( const V3 & vector ) const
{
	return V3( y * vector.z - z * vector.y, z * vector.x - x*vector.z, x * vector.y - y * vector.x);
}

//*************************************************************************************
//
//*************************************************************************************
V3 V3::UnitCross( const V3 & vector ) const
{
	V3	result( this->Cross( vector ) );

	result.Normalise();

	return result;
}

//*************************************************************************************
//
//*************************************************************************************
bool V3::IsParallel( const V3 & vector ) const
{
	return FABSF( Cross( vector ).LengthSq() ) < EPSILON;
}

//*************************************************************************************
//
//*************************************************************************************
V3 V3::Perpendicular() const
{
	static const V3	up( 0.f, 1.f, 0.f );
	static const V3	right( 1.f, 0.f, 0.f );

	if ( IsParallel( up ) )
	{
		return UnitCross( right );
	}
	else
	{
		return UnitCross( up );
	}
}

//*************************************************************************************
//
//*************************************************************************************
V3 V3::Minimise( const V3 & vector ) const
{
	V3	result;

	result.x = MIN( x, vector.x );
	result.y = MIN( y, vector.y );
	result.z = MIN( z, vector.z );

	return result;
}

V3 V3::Maximise( const V3 &vector ) const
{
	V3	result;

	result.x = MAX( x, vector.x );
	result.y = MAX( y, vector.y );
	result.z = MAX( z, vector.z );

	return result;
}

//*************************************************************************************
//
//*************************************************************************************
V3 V3::Lerp( const V3 & vector, const float weight ) const
{
	return (*this + (weight * (vector - *this)));
}

//*********************************************************************************
//	Vector3 Operators
//*********************************************************************************
inline V3 operator * (const float fScalar, const V3 &vec)
{
	return V3( fScalar * vec.x, fScalar * vec.y, fScalar * vec.z );
}

inline V3 operator + (const V3 &vec, const float &val)
{
	return V3( vec.x + val, vec.y + val, vec.z + val );
}

inline V3 operator - (const V3 &vec)
{
	return V3( -vec.x, -vec.y, -vec.z );
}

inline V3 operator - (const V3 &vec, const float &val)
{
	return V3( vec.x - val, vec.y - val, vec.z - val );
}

//**********************************************************************************
//
//   V4 Class Implenentation
//
//**********************************************************************************
V4::V4()
{
}

V4::V4( const V4 & src ) : x( src.x ), y( src.y ), z( src.z ), w( src.w )
{
}

V4::V4( const V4 * const p_src ) : x( p_src->x ), y( p_src->y ), z( p_src->z ), w( p_src->w )
{
}

V4::V4( const float ix, const float iy, const float iz, const float iw ) : x( ix ), y( iy ), z( iz ), w( iw )
{
}

//*************************************************************************************
//
//*************************************************************************************
V4 & V4::operator = (const V4 & vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;
	w = vec.w;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V4 V4::operator + (const V4 & rhs) const
{
	return V4( x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w );
}

V4 & V4::operator += (const V4 & vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;
	w += vector.w;

	return *this;
}

V4 & V4::operator += (const float &val)
{
	x += val;
	y += val;
	z += val;
	w += val;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V4 V4::operator - (const V4 & rhs) const
{
	return V4 (x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

V4 & V4::operator -= (const V4 & vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;
	w -= vector.w;

	return *this;
}

V4 & V4::operator -= (const float & val)
{
	x -= val;
	y -= val;
	z -= val;
	w -= val;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
const V4 V4::operator * (const float rhs) const
{
	return V4( x * rhs, y * rhs, z * rhs, w * rhs );
}

V4 & V4::operator *= (const float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;
	w *= fScalar;

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
V4 & V4::operator /= (const float fScalar)
{
	if (fScalar != 0.f)
	{
		*this *= (1.f / fScalar);
	}
	else
	{
		x = FLT_MAX;
		y = FLT_MAX;
		z = FLT_MAX;
		w = FLT_MAX;
	}

	return *this;
}

//*************************************************************************************
//
//*************************************************************************************
float V4::Length() const
{
	return SQRTF( LengthSq() );
}

float V4::LengthSq() const
{
	return x*x + y*y + z*z + w*w;
}

//*************************************************************************************
//
//*************************************************************************************
float V4::Dot( const V4 &vector ) const
{
	return (x * vector.x + y * vector.y + z * vector.z + w * vector.w);
}

//*************************************************************************************
//
//*************************************************************************************
float V4::Normalise()
{
	float length( Length() );

	if (length > FLT_MIN)
	{
		*this *= ( 1.f / length );
	}
	else
	{
		length = 0.f;
	}

	return length;
}

//*************************************************************************************
//
//*************************************************************************************
V4 V4::Normal() const
{
	V4	result( *this );

	result.Normalise();

	return result;
}

//*************************************************************************************
//
//*************************************************************************************
V4 V4::Cross( const V4 & vector1, const V4 & vector2 ) const
{
	// Calculate intermediate values.
	const float	A = (vector1.x * vector2.y) - (vector1.y * vector2.x);
	const float	B = (vector1.x * vector2.z) - (vector1.z * vector2.x);
	const float	C = (vector1.x * vector2.w) - (vector1.w * vector2.x);
	const float	D = (vector1.y * vector2.z) - (vector1.z * vector2.y);
	const float	E = (vector1.y * vector2.w) - (vector1.w * vector2.y);
	const float	F = (vector1.z * vector2.w) - (vector1.w * vector2.z);

	// Calculate the result-vector components.
	return V4(+(y * F) - (z * E) + (w * D),
		-(x * F) + (z * C) - (w * B),
		+(x * E) - (y * C) + (w * A),
		-(x * D) + (y * B) - (z * A));
}

//*************************************************************************************
//
//*************************************************************************************
V4 V4::UnitCross( const V4 &vector1, const V4 &vector2 ) const
{
	V4	result( this->Cross( vector1, vector2 ) );

	result.Normalise();

	return result;
}

//*************************************************************************************
//
//*************************************************************************************
V4 V4::Lerp( const V4 &vector, const float weight ) const
{
	return (*this + (weight * (vector - *this)));
}

//*********************************************************************************
//	Vector4 Operators
//*********************************************************************************

inline V4 operator * (const float fScalar, const V4 &vec)
{
	return V4( fScalar * vec.x, fScalar * vec.y, fScalar * vec.z, fScalar * vec.w );
}

inline V4 operator + (const V4 &vec, const float &val)
{
	return V4( vec.x + val, vec.y + val, vec.z + val, vec.w + val );
}

inline V4 operator - (const V4 &vec)
{
	return V4( -vec.x, -vec.y, -vec.z, -vec.w );
}

inline V4 operator - (const V4 &vec, const float &val)
{
	return V4( vec.x - val, vec.y - val, vec.z - val, vec.w - val );
}


//*************************************************************************************
//
//	Utility functions
//
//*************************************************************************************

//*************************************************************************************
//
//*************************************************************************************
V2	Rotate( const V2 & v, float y_angle )
{
	const float	c( cosf( y_angle ) );
	const float	s( sinf( y_angle ) );

	return V2( v.x * c - v.y * s, v.x * s + v.y * c );
}

//*************************************************************************************
//
//*************************************************************************************
V3	RotateY( const V3 & v, float y_angle )
{
	const float	c( cosf( y_angle ) );
	const float	s( sinf( y_angle ) );

	return V3( v.x * c - v.z * s, v.y, v.x * s + v.z * c );
}

//*************************************************************************************
//
//*************************************************************************************
V3	RotateX( const V3 & v, float x_angle )
{
	const float	c( cosf( x_angle ) );
	const float	s( sinf( x_angle ) );

	return V3( v.x, v.y * c + v.z * s, -v.y * s + v.z * c );
}

//*************************************************************************************
//
//*************************************************************************************
V3	RotateZ( const V3 & v, float z_angle )
{
	const float	c( cosf( z_angle ) );
	const float	s( sinf( z_angle ) );

	return V3( v.x * c + v.y * s, -v.x * s + v.y * c, v.z );
}


//*******************************  END OF FILE  ************************************
