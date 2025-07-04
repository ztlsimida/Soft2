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

 WebRtcIlbcfix_SimpleLsfQ.c

******************************************************************/

#include "simple_lsf_quant.h"

#include "constants.h"
#include "defines.h"
#include "split_vq.h"

/*----------------------------------------------------------------*
 *  lsf quantizer (subrutine to LPCencode)
 *---------------------------------------------------------------*/

void WebRtcIlbcfix_SimpleLsfQ(
    int16_t *lsfdeq, /* (o) dequantized lsf coefficients
                                   (dimension FILTERORDER) Q13 */
    int16_t *index, /* (o) quantization index */
    int16_t *lsf, /* (i) the lsf coefficient vector to be
                           quantized (dimension FILTERORDER) Q13 */
    int16_t lpc_n /* (i) number of lsf sets to quantize */
                              ){

  /* Quantize first LSF with memoryless split VQ */
  WebRtcIlbcfix_SplitVq( lsfdeq, index, lsf,
                         (int16_t*)WebRtcIlbcfix_kLsfCb, (int16_t*)WebRtcIlbcfix_kLsfDimCb, (int16_t*)WebRtcIlbcfix_kLsfSizeCb);

  if (lpc_n==2) {
    /* Quantize second LSF with memoryless split VQ */
    WebRtcIlbcfix_SplitVq( lsfdeq + LPC_FILTERORDER, index + LSF_NSPLIT,
                           lsf + LPC_FILTERORDER, (int16_t*)WebRtcIlbcfix_kLsfCb,
                           (int16_t*)WebRtcIlbcfix_kLsfDimCb, (int16_t*)WebRtcIlbcfix_kLsfSizeCb);
  }
  return;
}
