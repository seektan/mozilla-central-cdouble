/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Archive code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef MAR_PRIVATE_H__
#define MAR_PRIVATE_H__

#include "prtypes.h"

#define BLOCKSIZE 4096
#define ROUND_UP(n, incr) (((n) / (incr) + 1) * (incr))

#define MAR_ID "MAR1"
#define MAR_ID_SIZE 4
#define SIGNATURE_BLOCK_OFFSET 16

#define MAR_ITEM_SIZE(namelen) (3*sizeof(PRUint32) + (namelen) + 1)

#if defined WORDS_BIGENDIAN || defined IS_BIG_ENDIAN
#define NETWORK_TO_HOST64(x) (x)
#define HOST_TO_NETWORK64(x) (x)
#else
#define HOST_TO_NETWORK64(x) ( \
  ((((PRUint64) x) & 0xFF) << 56) | \
  ((((PRUint64) x) >> 8) & 0xFF) << 48) | \
  (((((PRUint64) x) >> 16) & 0xFF) << 40) | \
  (((((PRUint64) x) >> 24) & 0xFF) << 32) | \
  (((((PRUint64) x) >> 32) & 0xFF) << 24) | \
  (((((PRUint64) x) >> 40) & 0xFF) << 16) | \
  (((((PRUint64) x) >> 48) & 0xFF) << 8) | \
  (((PRUint64) x) >> 56)
#define NETWORK_TO_HOST64 HOST_TO_NETWORK64
#endif

#endif  /* MAR_PRIVATE_H__ */
