/**************************************************************************
	Souliss
    Copyright (C) 2011  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Modified by Dario Di Maio
	Updated by Luca Calcaterra for ESP32
	
***************************************************************************/

/**************************************************************************
 *
 * Function:    halfprecision
 * Filename:    halfprecision.c
 * Programmer:  James Tursa
 * Version:     1.0
 * Date:        March 3, 2009
 * Copyright:   (c) 2009 by James Tursa, All Rights Reserved
 *
 *  This code uses the BSD License:
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are 
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the distribution
 *      
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * halfprecision converts the input argument to/from a half precision floating
 * point bit pattern corresponding to IEEE 754r. The bit pattern is stored in a
 * uint16 class variable. Please note that halfprecision is *not* a class. That
 * is, you cannot do any arithmetic with the half precision bit patterns.
 * halfprecision is simply a function that converts the IEEE 754r half precision
 * bit pattern to/from other numeric MATLAB variables. You can, however, take
 * the half precision bit patterns, convert them to single or double, do the
 * operation, and then convert the result back manually.
 *
 * Note that the special cases of -Inf, +Inf, and NaN are handled correctly.
 * Also, note that the -1e30 and 1e30 values overflow the half precision format
 * and are converted into half precision -Inf and +Inf values, and stay that
 * way when they are converted back into doubles.
 *
 * For the denormalized cases, note that 2^(-24) is the smallest number that can
 * be represented in half precision exactly. 2^(-25) will convert to 2^(-24)
 * because of the rounding algorithm used, and 2^(-26) is too small and underflows
 * to zero.
 *
 ***************************************************************************/

/*!
    \file 
    \ingroup

*/ 

/**************************************************************************/
/*!
	Convert a single precision floating point into an half precision one
*/	
/**************************************************************************/
void float16(uint16_t *output, float *input)
{
	uint16_t *hp = (uint16_t *)output;
	uint16_t hs, he, hm;
	uint32_t x, xs, xe, xm;
	int hes;

	// Utilizzo di memcpy al posto di cast diretti
	memcpy(&x, input, sizeof(x));

	if ((x & 0x7FFFFFFF) == 0)
	{
		*hp = 0x8000;
	}
	else
	{
		xs = x & 0x80000000;
		xe = x & 0x7F800000;
		xm = x & 0x007FFFFF;

		if (xe == 0)
		{
			*hp = (uint16_t)(xs >> 16);
		}
		else if (xe == 0x7F800000)
		{
			if (xm == 0)
				*hp = (uint16_t)((xs >> 16) | 0x7C00);
			else
				*hp = (uint16_t)0xFE00;
		}
		else
		{
			hs = (uint16_t)(xs >> 16);
			hes = ((int)(xe >> 23)) - 127 + 15;
			if (hes >= 0x1F)
				*hp = (uint16_t)((xs >> 16) | 0x7C00);
			else if (hes <= 0)
			{
				if ((14 - hes) > 24)
					hm = (uint16_t)0;
				else
				{
					xm |= 0x00800000;
					hm = (uint16_t)(xm >> (14 - hes));
					if ((xm >> (13 - hes)) & 0x00000001)
						hm += (uint16_t)1;
				}

				*hp = (hs | hm);
			}
			else
			{
				he = (uint16_t)(hes << 10);
				hm = (uint16_t)(xm >> 13);
				if (xm & 0x00001000)
					*hp = (hs | he | hm) + (uint16_t)1;
				else
					*hp = (hs | he | hm);
			}
		}
	}
}

/**************************************************************************/
/*!
	Convert an half precision floating point into an single precision one
*/	
/**************************************************************************/
void float32(uint16_t *input, float *output)
{
	uint16_t *hp = (uint16_t *)input;
	uint32_t x, xs, xe, xm;
	uint16_t h, hs, he, hm;
	int32_t xes;
	int e;

	memcpy(&h, hp, sizeof(h));

	if ((h & 0x7FFF) == 0)
	{
		x = ((uint32_t)h) << 16;
	}
	else
	{
		hs = h & 0x8000;
		he = h & 0x7C00;
		hm = h & 0x03FF;

		if (he == 0)
		{
			e = -1;
			do
			{
				e++;
				hm <<= 1;
			} while ((hm & 0x0400) == 0);

			xs = ((uint32_t)hs) << 16;
			xes = ((int32_t)(he >> 10)) - 15 + 127 - e;
			xe = (uint32_t)(xes << 23);
			xm = ((uint32_t)(hm & 0x03FF)) << 13;
			x = (xs | xe | xm);
		}
		else if (he == 0x7C00)
		{
			if (hm == 0)
				x = (((uint32_t)hs) << 16) | ((uint32_t)0x7F800000);
			else
				x = (uint32_t)0xFFC00000;
		}
		else
		{
			xs = ((uint32_t)hs) << 16;
			xes = ((int32_t)(he >> 10)) - 15 + 127;
			xe = (uint32_t)(xes << 23);
			xm = ((uint32_t)hm) << 13;
			x = (xs | xe | xm);
		}
	}

	// Copia il risultato nella variabile di output
	memcpy(output, &x, sizeof(x));
}

float returnfloat32(uint16_t *input)
{
	float output;
	float32(input, &output);
	return output;
}

uint16_t returnfloat16(float *input)
{
	uint16_t output;
	float16(&output, input);
	return output;
}