/***********************************************************************************

  Module :	CVector.h

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

#ifndef CVECTOR_H_
#define CVECTOR_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
struct P2
{
	float	x;
	float	y;
};

struct P3
{
	float	x;
	float	y;
	float	z;
};

struct P4
{
	float	x;
	float	y;
	float	z;
	float	w;
};

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//*********************************************************************************
//*********************************************************************************
//
//	V2 class definition
//
//*********************************************************************************
//*********************************************************************************
class V2
{
	public:

		V2();
		V2( const V2 & src );
		V2( const float ix, const float iy );
		V2( const V2 * const p_src );

	public:

		V2 &			operator  = ( const V2 & vec );

		const V2		operator  + ( const V2 & rhs ) const;
		friend V2		operator  + ( const V2 & vector, const float & val );

		V2 &			operator += ( const V2 & vector );

		const V2		operator  - ( const V2 & rhs ) const;
		friend V2		operator  - ( const V2 & vector );
		friend V2		operator  - ( const V2 & vector, const float & val );

		V2 &			operator -= ( const V2 & vector );

		const V2		operator  * ( const float rhs ) const;
		friend V2		operator  * ( const float fScalar, const V2 & vector );

		V2 &			operator *= ( const float fScalar );

		V2 &			operator /= ( const float fScalar );

	public:

		float			Length() const;
		float			LengthSq() const;

		float			Dot( const V2 & vector ) const;

		float			Angle() const;

		float			Normalise( const float tolerance = 1e-06 );

		V2				Lerp( const V2 &vector, const float weight ) const;

		float			Distance( const V2 & vector ) const;
		float			DistanceSq( const V2 & vector ) const;

	public:

		float			x;
		float			y;
};


//*********************************************************************************
//*********************************************************************************
//
//	V3 class definition
//
//*********************************************************************************
//*********************************************************************************
class V3
{
	public:

		V3();
		V3( const V3 & src );
		V3( const V3 * const p_src );
		V3( const float ix, const float iy, const float iz );
		V3( const float * const p_src );

	public:

		operator float * ();
		operator const float * () const;

		V3				operator  + () const;
		V3				operator  - () const;

		V3 &			operator  = ( const V3 & vec );

		const V3		operator  + ( const V3 & rhs ) const;
		friend V3		operator  + ( const V3 & vector, const float & val );

		V3 &			operator += ( const float & val );
		V3 &			operator += ( const V3 & vector );

		const V3		operator  - ( const V3 & rhs ) const;

		V3 &			operator -= ( const float & val );
		V3 &			operator -= ( const V3 & vector );

		const V3		operator  * ( const float rhs ) const;
		friend V3		operator  * ( const float fScalar, const V3 & vector );

		V3 &			operator *= ( const float fScalar );

		V3 &			operator /= ( const float fScalar );

		bool			operator == ( const V3 & rhs ) const;
		bool			operator != ( const V3 & rhs ) const;

		bool			operator  > ( const V3 & rhs ) const;
		bool			operator  < ( const V3 & rhs ) const;

		bool			operator >= ( const V3 & rhs ) const;
		bool			operator <= ( const V3 & rhs ) const;

	public:

		float			Length() const;
		float			LengthSq() const;

		float			Distance( const V3 & vector ) const;
		float			DistanceSq( const V3 & vector ) const;

		float			HorizontalDistance() const;
		float			HorizontalDistanceSq() const;

		float			HorizontalDistance( const V3 & vector ) const;
		float			HorizontalDistanceSq( const V3 & vector ) const;

		float			Dot( const V3 & vector ) const;

		float			Angle() const;
		float			Pitch() const;

		bool			IsNormalised() const;

		float			Normalise();

		V3				Normal() const;

		V3				Cross( const V3 & vector ) const;

		V3				UnitCross( const V3 & vector ) const;

		bool			IsParallel( const V3 & vector ) const ;

		V3				Perpendicular() const;

		V3				Minimise( const V3 & vector ) const;
		V3				Maximise( const V3 & vector ) const;

		V3				Lerp( const V3 & vector, const float weight ) const;

	public:

		float			x;
		float			y;
		float			z;
};


//*********************************************************************************
//*********************************************************************************
//
//	V4 class definition
//
//*********************************************************************************
//*********************************************************************************
class V4
{
	public:

		V4();
		V4( const V4 & src );
		V4( const V4 * const p_src );
		V4( const float x, const float y, const float z, const float w );

	public:

		V4 &			operator  = ( const V4 & vec );

		const V4		operator  + ( const V4 & rhs) const;
		friend V4		operator  + ( const V4 & vector, const float & val );

		V4 &			operator += ( const float & val );
		V4 &			operator += ( const V4 & vector );

		const V4		operator  - ( const V4 & rhs ) const;
		friend V4		operator  - ( const V4 & vector );
		friend V4		operator  - ( const V4 & vector, const float & val );

		V4 &			operator -= ( const float & val );
		V4 &			operator -= ( const V4 & vector );

		const V4		operator  * ( const float rhs ) const;
		friend V4		operator  * ( const float fScalar, const V4 & vector );

		V4 &			operator *= ( const float fScalar );

		V4 &			operator /= ( const float fScalar );

	public:

		float			Length() const;
		float			LengthSq() const;

		float			Dot( const V4 & vector ) const;

		float			Normalise();

		V4				Normal() const;

		V4				Cross( const V4 & vector1, const V4 & vector2 ) const;

		V4				UnitCross( const V4 & vector1, const V4 &vector2 ) const;

		V4				Lerp( const V4 & vector, const float weight ) const;

	public:

		float			x;
		float			y;
		float			z;
		float			w;
};


//*********************************************************************************
//	Externs
//*********************************************************************************
const V3	gZeroVector( 0.f, 0.f, 0.f );
const V3	gAtVector( 0.f, 0.f, 1.f );
const V3	gForwardVector( 0.f, 0.f, 1.f );
const V3	gUpVector( 0.f, 1.f, 0.f );
const V3	gRightVector( 1.f, 0.f, 0.f );

const V4	gZeroVector4( 0.0f, 0.0f, 0.0f, 0.0f );
const V4	gOneVector4( 1.0f, 1.0f, 1.0f, 1.0f );
const V4	gHalfVector4( 0.5f, 0.5f, 0.5f, 0.5f );

//*********************************************************************************
//	Prototypes
//*********************************************************************************
V2	Rotate( const V2 & v, float y_angle );
V3	RotateY( const V3 & v, float y_angle );
V3	RotateX( const V3 & v, float x_angle );
V3	RotateZ( const V3 & v, float z_angle );

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* _VECTOR_H_ */
