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

#include "mozilla/ModuleUtils.h"

#include "nsICharsetAlias.h"
#include "nsCOMPtr.h"

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsUniversalCharDetDll.h"
#include "nsISupports.h"
#include "nsICategoryManager.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"

#include "nsUniversalDetector.h"
#include "nsUdetXPCOMWrapper.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMStringDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHTWPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHTWStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHCNPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHCNStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCJKPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCJKStringPSMDetector)
NS_DEFINE_NAMED_CID(NS_UNIVERSAL_DETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_UNIVERSAL_STRING_DETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_JA_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_JA_STRING_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_KO_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_KO_STRING_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZHTW_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZHTW_STRING_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZHCN_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZHCN_STRING_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZH_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_ZH_STRING_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_CJK_PSMDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_CJK_STRING_PSMDETECTOR_CID);

static const mozilla::Module::CIDEntry kChardetCIDs[] = {
  { &kNS_UNIVERSAL_DETECTOR_CID, false, NULL, nsUniversalXPCOMDetectorConstructor },
  { &kNS_UNIVERSAL_STRING_DETECTOR_CID, false, NULL, nsUniversalXPCOMStringDetectorConstructor },
  { &kNS_JA_PSMDETECTOR_CID, false, NULL, nsJAPSMDetectorConstructor },
  { &kNS_JA_STRING_PSMDETECTOR_CID, false, NULL, nsJAStringPSMDetectorConstructor },
  { &kNS_KO_PSMDETECTOR_CID, false, NULL, nsKOPSMDetectorConstructor },
  { &kNS_KO_STRING_PSMDETECTOR_CID, false, NULL, nsKOStringPSMDetectorConstructor },
  { &kNS_ZHTW_PSMDETECTOR_CID, false, NULL, nsZHTWPSMDetectorConstructor },
  { &kNS_ZHTW_STRING_PSMDETECTOR_CID, false, NULL, nsZHTWStringPSMDetectorConstructor },
  { &kNS_ZHCN_PSMDETECTOR_CID, false, NULL, nsZHCNPSMDetectorConstructor },
  { &kNS_ZHCN_STRING_PSMDETECTOR_CID, false, NULL, nsZHCNStringPSMDetectorConstructor },
  { &kNS_ZH_PSMDETECTOR_CID, false, NULL, nsZHPSMDetectorConstructor },
  { &kNS_ZH_STRING_PSMDETECTOR_CID, false, NULL, nsZHStringPSMDetectorConstructor },
  { &kNS_CJK_PSMDETECTOR_CID, false, NULL, nsCJKPSMDetectorConstructor },
  { &kNS_CJK_STRING_PSMDETECTOR_CID, false, NULL, nsCJKStringPSMDetectorConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kChardetContracts[] = {
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector", &kNS_UNIVERSAL_DETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "universal_charset_detector", &kNS_UNIVERSAL_STRING_DETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "ja_parallel_state_machine", &kNS_JA_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "ja_parallel_state_machine", &kNS_JA_STRING_PSMDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "ko_parallel_state_machine", &kNS_KO_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "ko_parallel_state_machine", &kNS_KO_STRING_PSMDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhtw_parallel_state_machine", &kNS_ZHTW_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "zhtw_parallel_state_machine", &kNS_ZHTW_STRING_PSMDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhcn_parallel_state_machine", &kNS_ZHCN_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "zhcn_parallel_state_machine", &kNS_ZHCN_STRING_PSMDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "zh_parallel_state_machine", &kNS_ZH_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "zh_parallel_state_machine", &kNS_ZH_STRING_PSMDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "cjk_parallel_state_machine", &kNS_CJK_PSMDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "cjk_parallel_state_machine", &kNS_CJK_STRING_PSMDETECTOR_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kChardetCategories[] = {
  { NS_CHARSET_DETECTOR_CATEGORY, "universal_charset_detector", NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector" },
  { NS_CHARSET_DETECTOR_CATEGORY, "ja_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "ja_parallel_state_machine" },
  { NS_CHARSET_DETECTOR_CATEGORY, "ko_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "ko_parallel_state_machine" },
  { NS_CHARSET_DETECTOR_CATEGORY, "zhtw_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhtw_parallel_state_machine" },
  { NS_CHARSET_DETECTOR_CATEGORY, "zhcn_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhcn_parallel_state_machine" },
  { NS_CHARSET_DETECTOR_CATEGORY, "zh_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "zh_parallel_state_machine" },
  { NS_CHARSET_DETECTOR_CATEGORY, "cjk_parallel_state_machine", NS_CHARSET_DETECTOR_CONTRACTID_BASE "cjk_parallel_state_machine" },
  { NULL }
};

static const mozilla::Module kChardetModule = {
  mozilla::Module::kVersion,
  kChardetCIDs,
  kChardetContracts,
  kChardetCategories
};

NSMODULE_DEFN(nsUniversalCharDetModule) = &kChardetModule;
