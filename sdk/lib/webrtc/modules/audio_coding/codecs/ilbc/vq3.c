/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/******************************************************************

 iLBC Speech Coder ANSI-C Source Code

 WebRtcIlbcfix_Vq3.c

******************************************************************/

#include "vq3.h"

#include "constants.h"

/*----------------------------------------------------------------*
 *  vector quantization
 *---------------------------------------------------------------*/

void WebRtcIlbcfix_Vq3(
    int16_t *Xq, /* quantized vector (Q13) */
    int16_t *index,
    int16_t *CB, /* codebook in Q13 */
    int16_t *X,  /* vector to quantize (Q13) */
    int16_t n_cb
                       ){
  int16_t i, j;
  int16_t pos, minindex=0;
  int16_t tmp;
  int32_t dist, mindist;

  pos = 0;
  mindist = WEBRTC_SPL_WORD32_MAX; /* start value */

  /* Find the codebook with the lowest square distance */
  for (j = 0; j < n_cb; j++) {
    tmp = X[0] - CB[pos];
    dist = tmp * tmp;
    for (i = 1; i < 3; i++) {
      tmp = X[i] - CB[pos + i];
      dist += tmp * tmp;
    }

    if (dist < mindist) {
      mindist = dist;
      minindex = j;
    }
    pos += 3;
  }

  /* Store the quantized codebook and the index */
  for (i = 0; i < 3; i++) {
    Xq[i] = CB[minindex*3 + i];
  }
  *index = minindex;

}
