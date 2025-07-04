/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "noise_suppression_x.h"
#include "nsx_core.h"
#include "nsx_defines.h"

static const s16 kIndicatorTable[17] = 
{
    0, 2017, 3809, 5227, 6258, 6963, 7424, 7718,
    7901, 8014, 8084, 8126, 8152, 8168, 8177, 8183, 8187
};

// Compute speech/noise probability
// speech/noise probability is returned in: probSpeechFinal
//snrLocPrior is the prior SNR for each frequency (in Q11)
//snrLocPost is the post SNR for each frequency (in Q11)
void WebRtcNsx_SpeechNoiseProb(NoiseSuppressionFixedC* inst,
                                  u16* nonSpeechProbFinal,
                                  u32* priorLocSnr,
                                  u32* postLocSnr)
{
    u32 zeros, num, den, tmpU32no1, tmpU32no2, tmpU32no3;
    s32 invLrtFX, indPriorFX, tmp32, tmp32no1, tmp32no2, besselTmpFX32;
    s32 frac32, logTmp;
    s32 logLrtTimeAvgKsumFX;
    s16 indPriorFX16;
    s16 tmp16, tmp16no1, tmp16no2, tmpIndFX, tableIndex, frac, intPart;
    uint i;
    int normTmp, normTmp2, nShifts;

    // compute feature based on average LR factor
    // this is the average over all frequencies of the smooth log LRT
    logLrtTimeAvgKsumFX = 0;
    for (i = 0; i < NR_MAGNLEN; i++) {
        besselTmpFX32 = (s32) postLocSnr[i]; // Q11
        normTmp = WebRtcSpl_NormU32(postLocSnr[i]);
        num = postLocSnr[i] << normTmp; // Q(11+normTmp)
        if (normTmp > 10) {
            den = priorLocSnr[i] << (normTmp - 11); // Q(normTmp)
        } else {
            den = priorLocSnr[i] >> (11 - normTmp); // Q(normTmp)
        }
        if (den > 0) {
            besselTmpFX32 -= num / den; // Q11
        } else {
            besselTmpFX32 = 0;
        }

        // inst->logLrtTimeAvg[i] += LRT_TAVG * (besselTmp - log(snrLocPrior)
        //                                       - inst->logLrtTimeAvg[i]);
        // Here, LRT_TAVG = 0.5
        zeros = WebRtcSpl_NormU32(priorLocSnr[i]);
        frac32 = (s32) (((priorLocSnr[i] << zeros) & 0x7FFFFFFF) >> 19);
        tmp32 = (frac32 * frac32 * -43) >> 19;
        tmp32 += ((s16) frac32 * 5412) >> 12;
        frac32 = tmp32 + 37;
        // tmp32 = log2(priorLocSnr[i])
        tmp32 = (s32) (((31 - zeros) << 12) + frac32) - (11 << 12); // Q12
        logTmp = (tmp32 * 178) >> 8; // log2(priorLocSnr[i])*log(2)
        // tmp32no1 = LRT_TAVG * (log(snrLocPrior) + inst->logLrtTimeAvg[i]) in Q12.
        tmp32no1 = (logTmp + inst->logLrtTimeAvgW32[i]) / 2;
        inst->logLrtTimeAvgW32[i] += (besselTmpFX32 - tmp32no1); // Q12

        logLrtTimeAvgKsumFX += inst->logLrtTimeAvgW32[i]; // Q12
    }
    inst->featureLogLrt = (logLrtTimeAvgKsumFX * BIN_SIZE_LRT) >> (NR_STAGES + 11);

    // done with computation of LR factor

    //
    //compute the indicator functions
    //

    // average LRT feature
    // FLOAT code
    // indicator0 = 0.5 * (tanh(widthPrior *
    //                      (logLrtTimeAvgKsum - threshPrior0)) + 1.0);
    tmpIndFX = 16384; // Q14(1.0)
    tmp32no1 = logLrtTimeAvgKsumFX - inst->thresholdLogLrt; // Q12
    nShifts = 7 - NR_STAGES; // WIDTH_PR_MAP_SHIFT - inst->stages + 5;
    //use larger width in tanh map for pause regions
    if (tmp32no1 < 0) {
        tmpIndFX = 0;
        tmp32no1 = -tmp32no1;
        //widthPrior = widthPrior * 2.0;
        nShifts++;
    }
    tmp32no1 = WEBRTC_SPL_SHIFT_W32(tmp32no1, nShifts); // Q14
    // compute indicator function: sigmoid map
    if (tmp32no1 < (16 << 14) && tmp32no1 >= 0) {
        tableIndex = (s16) (tmp32no1 >> 14);
        tmp16no2 = kIndicatorTable[tableIndex];
        tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
        frac = (s16) (tmp32no1 & 0x00003fff); // Q14
        tmp16no2 += (s16) ((tmp16no1 * frac) >> 14);
        if (tmpIndFX == 0) {
            tmpIndFX = 8192 - tmp16no2; // Q14
        } else {
            tmpIndFX = 8192 + tmp16no2; // Q14
        }
    }
    indPriorFX = inst->weightLogLrt * tmpIndFX; // 6*Q14

    //spectral flatness feature
    if (inst->weightSpecFlat) {
        tmpU32no1 = WEBRTC_SPL_UMUL(inst->featureSpecFlat, 400); // Q10
        tmpIndFX = 16384; // Q14(1.0)
        //use larger width in tanh map for pause regions
        tmpU32no2 = inst->thresholdSpecFlat - tmpU32no1; //Q10
        nShifts = 4;
        if (inst->thresholdSpecFlat < tmpU32no1) {
            tmpIndFX = 0;
            tmpU32no2 = tmpU32no1 - inst->thresholdSpecFlat;
            //widthPrior = widthPrior * 2.0;
            nShifts++;
        }
        tmpU32no1 = WebRtcSpl_DivU32U16(tmpU32no2 << nShifts, 25); // Q14
        // compute indicator function: sigmoid map
        // FLOAT code
        // indicator1 = 0.5 * (tanh(sgnMap * widthPrior *
        //                          (threshPrior1 - tmpFloat1)) + 1.0);
        if (tmpU32no1 < (16 << 14)) {
            tableIndex = (s16) (tmpU32no1 >> 14);
            tmp16no2 = kIndicatorTable[tableIndex];
            tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
            frac = (s16) (tmpU32no1 & 0x00003fff); // Q14
            tmp16no2 += (s16) ((tmp16no1 * frac) >> 14);
            if (tmpIndFX) {
                tmpIndFX = 8192 + tmp16no2; // Q14
            } else {
                tmpIndFX = 8192 - tmp16no2; // Q14
            }
        }
        indPriorFX += inst->weightSpecFlat * tmpIndFX; // 6*Q14
    }

    //for template spectral-difference
    if (inst->weightSpecDiff) {
        tmpU32no1 = 0;
        if (inst->featureSpecDiff) {
            normTmp = WEBRTC_SPL_MIN(20 - NR_STAGES,
                    WebRtcSpl_NormU32(inst->featureSpecDiff));
            tmpU32no1 = inst->featureSpecDiff << normTmp; // Q(normTmp-2*stages)
            tmpU32no2 = inst->timeAvgMagnEnergy >> (20 - NR_STAGES - normTmp);
            if (tmpU32no2 > 0) {
                // Q(20 - inst->stages)
                tmpU32no1 /= tmpU32no2;
            } else {
                tmpU32no1 = (u32) (0x7fffffff);
            }
        }
        tmpU32no3 = (inst->thresholdSpecDiff << 17) / 25;
        tmpU32no2 = tmpU32no1 - tmpU32no3;
        nShifts = 1;
        tmpIndFX = 16384; // Q14(1.0)
        //use larger width in tanh map for pause regions
        if (tmpU32no2 & 0x80000000) {
            tmpIndFX = 0;
            tmpU32no2 = tmpU32no3 - tmpU32no1;
            //widthPrior = widthPrior * 2.0;
            nShifts--;
        }
        tmpU32no1 = tmpU32no2 >> nShifts;
        // compute indicator function: sigmoid map
        /* FLOAT code
         indicator2 = 0.5 * (tanh(widthPrior * (tmpFloat1 - threshPrior2)) + 1.0);
         */
        if (tmpU32no1 < (16 << 14)) {
            tableIndex = (s16) (tmpU32no1 >> 14);
            tmp16no2 = kIndicatorTable[tableIndex];
            tmp16no1 = kIndicatorTable[tableIndex + 1] - kIndicatorTable[tableIndex];
            frac = (s16) (tmpU32no1 & 0x00003fff); // Q14
            tmp16no2 += (s16) WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                    tmp16no1, frac, 14);
            if (tmpIndFX) {
                tmpIndFX = 8192 + tmp16no2;
            } else {
                tmpIndFX = 8192 - tmp16no2;
            }
        }
        indPriorFX += inst->weightSpecDiff * tmpIndFX; // 6*Q14
    }

    //combine the indicator function with the feature weights
    // FLOAT code
    // indPrior = 1 - (weightIndPrior0 * indicator0 + weightIndPrior1 *
    //                 indicator1 + weightIndPrior2 * indicator2);
    indPriorFX16 = WebRtcSpl_DivW32W16ResW16(98307 - indPriorFX, 6); // Q14
    // done with computing indicator function

    //compute the prior probability
    // FLOAT code
    // inst->priorNonSpeechProb += PRIOR_UPDATE *
    //                             (indPriorNonSpeech - inst->priorNonSpeechProb);
    tmp16 = indPriorFX16 - inst->priorNonSpeechProb; // Q14
    inst->priorNonSpeechProb += (s16) ((PRIOR_UPDATE_Q14 * tmp16) >> 14);

    //final speech probability: combine prior model with LR factor:

    memset(nonSpeechProbFinal, 0, sizeof (u16) * NR_MAGNLEN);

    if (inst->priorNonSpeechProb > 0) {
        for (i = 0; i < NR_MAGNLEN; i++) {
            // FLOAT code
            // invLrt = exp(inst->logLrtTimeAvg[i]);
            // invLrt = inst->priorSpeechProb * invLrt;
            // nonSpeechProbFinal[i] = (1.0 - inst->priorSpeechProb) /
            //                         (1.0 - inst->priorSpeechProb + invLrt);
            // invLrt = (1.0 - inst->priorNonSpeechProb) * invLrt;
            // nonSpeechProbFinal[i] = inst->priorNonSpeechProb /
            //                         (inst->priorNonSpeechProb + invLrt);
            if (inst->logLrtTimeAvgW32[i] < 65300) {
                tmp32no1 = (inst->logLrtTimeAvgW32[i] * 23637) >> 14; // Q12
                intPart = (s16) (tmp32no1 >> 12);
                if (intPart < -8) {
                    intPart = -8;
                }
                frac = (s16) (tmp32no1 & 0x00000fff); // Q12

                // Quadratic approximation of 2^frac
                tmp32no2 = (frac * frac * 44) >> 19; // Q12.
                tmp32no2 += (frac * 84) >> 7; // Q12
                invLrtFX = (1 << (8 + intPart)) +
                            WEBRTC_SPL_SHIFT_W32(tmp32no2, intPart - 4); // Q8

                normTmp = WebRtcSpl_NormW32(invLrtFX);
                normTmp2 = WebRtcSpl_NormW16((16384 - inst->priorNonSpeechProb));
                if (normTmp + normTmp2 >= 7) {
                    if (normTmp + normTmp2 < 15) {
                        invLrtFX >>= 15 - normTmp2 - normTmp;
                        // Q(normTmp+normTmp2-7)
                        tmp32no1 = invLrtFX * (16384 - inst->priorNonSpeechProb);
                        // Q(normTmp+normTmp2+7)
                        invLrtFX = WEBRTC_SPL_SHIFT_W32(tmp32no1, 7 - normTmp - normTmp2);
                        // Q14
                    } else {
                        tmp32no1 = invLrtFX * (16384 - inst->priorNonSpeechProb);
                        // Q22
                        invLrtFX = tmp32no1 >> 8; // Q14.
                    }

                    tmp32no1 = (s32) inst->priorNonSpeechProb << 8; // Q22

                    nonSpeechProbFinal[i] = tmp32no1 /
                            (inst->priorNonSpeechProb + invLrtFX); // Q8
                }
            }
        }
    }
}
