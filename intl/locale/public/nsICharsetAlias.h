/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsICharsetAlias_h___
#define nsICharsetAlias_h___

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"

/* 0b4028d6-7473-4958-9b3c-4dee46bf68cb */
#define NS_ICHARSETALIAS_IID \
{   0x0b4028d6, \
    0x7473, \
    0x4958, \
    {0x9b, 0x3c, 0x4d, 0xee, 0x46, 0xbf, 0x68, 0xcb} }

// {98D41C21-CCF3-11d2-B3B1-00805F8A6670}
#define NS_CHARSETALIAS_CID \
{ 0x98d41c21, 0xccf3, 0x11d2, { 0xb3, 0xb1, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 }}

#define NS_CHARSETALIAS_CONTRACTID "@mozilla.org/intl/charsetalias;1"

class nsICharsetAlias : public nsISupports
{
public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARSETALIAS_IID)

   NS_IMETHOD GetPreferred(const nsACString& aAlias, nsACString& aResult) = 0;
   NS_IMETHOD Equals(const nsACString& aCharset1, const nsACString& aCharset2, bool* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharsetAlias, NS_ICHARSETALIAS_IID)

#endif /* nsICharsetAlias_h___ */
