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

#include "../SigProc_FIX.h"

/* Step up function, converts reflection coefficients to prediction coefficients */
void silk_k2a(
    opus_int32                  *A_Q24,             /* O    Prediction coefficients [order] Q24                         */
    const opus_int16            *rc_Q15,            /* I    Reflection coefficients [order] Q15                         */
    const opus_int32            order               /* I    Prediction order                                            */
)
{
    opus_int   k, n;
    opus_int32 rc, tmp1, tmp2;

    for( k = 0; k < order; k++ ) {
        rc = rc_Q15[ k ];
        for( n = 0; n < (k + 1) >> 1; n++ ) {
            tmp1 = A_Q24[ n ];
            tmp2 = A_Q24[ k - n - 1 ];
            A_Q24[ n ]         = silk_SMLAWB( tmp1, silk_LSHIFT( tmp2, 1 ), rc );
            A_Q24[ k - n - 1 ] = silk_SMLAWB( tmp2, silk_LSHIFT( tmp1, 1 ), rc );
        }
        A_Q24[ k ] = -silk_LSHIFT( (opus_int32)rc_Q15[ k ], 9 );
    }
}
