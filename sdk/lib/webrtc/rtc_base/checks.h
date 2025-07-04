/*
 *  Copyright 2006 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_CHECKS_H_
#define RTC_BASE_CHECKS_H_


#define RTC_DISABLE_CHECK_MSG
// If you for some reson need to know if DCHECKs are on, test the value of
// RTC_DCHECK_IS_ON. (Test its value, not if it's defined; it'll always be
// defined, to either a true or a false value.)
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define RTC_DCHECK_IS_ON 1
#else
#define RTC_DCHECK_IS_ON 0
#endif

void rtc_FatalMessage(const char* file, int line, const char* msg);

#ifdef RTC_DISABLE_CHECK_MSG
#define RTC_CHECK_MSG_ENABLED 0
#else
#define RTC_CHECK_MSG_ENABLED 1
#endif

#if RTC_CHECK_MSG_ENABLED
#define RTC_CHECK_EVAL_MESSAGE(message) message
#else
#define RTC_CHECK_EVAL_MESSAGE(message) ""
#endif

// C version. Lacks many features compared to the C++ version, but usage
// guidelines are the same.

#define RTC_CHECK(condition)                                                 \
  do {                                                                       \
    if (!(condition)) {                                                      \
      rtc_FatalMessage(__FILE__, __LINE__,                                   \
                       RTC_CHECK_EVAL_MESSAGE("CHECK failed: " #condition)); \
    }                                                                        \
  } while (0)

#define RTC_CHECK_EQ(a, b) RTC_CHECK((a) == (b))
#define RTC_CHECK_NE(a, b) RTC_CHECK((a) != (b))
#define RTC_CHECK_LE(a, b) RTC_CHECK((a) <= (b))
#define RTC_CHECK_LT(a, b) RTC_CHECK((a) < (b))
#define RTC_CHECK_GE(a, b) RTC_CHECK((a) >= (b))
#define RTC_CHECK_GT(a, b) RTC_CHECK((a) > (b))

#define RTC_DCHECK(condition)                                                 \
  do {                                                                        \
    if (RTC_DCHECK_IS_ON && !(condition)) {                                   \
      rtc_FatalMessage(__FILE__, __LINE__,                                    \
                       RTC_CHECK_EVAL_MESSAGE("DCHECK failed: " #condition)); \
    }                                                                         \
  } while (0)

#define RTC_DCHECK_EQ(a, b) RTC_DCHECK((a) == (b))
#define RTC_DCHECK_NE(a, b) RTC_DCHECK((a) != (b))
#define RTC_DCHECK_LE(a, b) RTC_DCHECK((a) <= (b))
#define RTC_DCHECK_LT(a, b) RTC_DCHECK((a) < (b))
#define RTC_DCHECK_GE(a, b) RTC_DCHECK((a) >= (b))
#define RTC_DCHECK_GT(a, b) RTC_DCHECK((a) > (b))

#endif  // RTC_BASE_CHECKS_H_
