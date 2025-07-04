/***********************************************************************
Copyright (c) 2006-2011, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of Internet Society, IETF or IETF Trust, nor the
names of specific contributors, may be used to endorse or promote
products derived from this software without specific prior written
permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#ifdef HAVE_OPUS_CONFIG_H
#include "opus_config.h"
#endif

#include "main.h"

/* Interpolate two vectors */
void silk_interpolate(
    opus_int16                  xi[ MAX_LPC_ORDER ],            /* O    interpolated vector                         */
    const opus_int16            x0[ MAX_LPC_ORDER ],            /* I    first vector                                */
    const opus_int16            x1[ MAX_LPC_ORDER ],            /* I    second vector                               */
    const opus_int              ifact_Q2,                       /* I    interp. factor, weight on 2nd vector        */
    const opus_int              d                               /* I    number of parameters                        */
)
{
    opus_int i;

    celt_assert( ifact_Q2 >= 0 );
    celt_assert( ifact_Q2 <= 4 );

    for( i = 0; i < d; i++ ) {
        xi[ i ] = (opus_int16)silk_ADD_RSHIFT( x0[ i ], silk_SMULBB( x1[ i ] - x0[ i ], ifact_Q2 ), 2 );
    }
}
