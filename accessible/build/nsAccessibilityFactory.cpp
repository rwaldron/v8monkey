/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
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

#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleRetrieval.h"
#include "nscore.h"

static nsresult
NS_ConstructAccessibilityService(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    nsIAccessibilityService* accessibility;
    rv = NS_GetAccessibilityService(&accessibility);
    if (NS_FAILED(rv)) {
        NS_ERROR("Unable to construct accessibility service");
        return rv;
    }
    rv = accessibility->QueryInterface(aIID, aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
    NS_RELEASE(accessibility);
    return rv;
}

NS_DEFINE_NAMED_CID(NS_ACCESSIBILITY_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_ACCESSIBLE_RETRIEVAL_CID);

static const mozilla::Module::CIDEntry kA11yCIDs[] = {
    { &kNS_ACCESSIBILITY_SERVICE_CID, false, NULL, NS_ConstructAccessibilityService },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kA11yContracts[] = {
    { "@mozilla.org/accessibilityService;1", &kNS_ACCESSIBILITY_SERVICE_CID },
    { "@mozilla.org/accessibleRetrieval;1", &kNS_ACCESSIBILITY_SERVICE_CID },
    { NULL }
};

static const mozilla::Module kA11yModule = {
    mozilla::Module::kVersion,
    kA11yCIDs,
    kA11yContracts
};

NSMODULE_DEFN(nsAccessibilityModule) = &kA11yModule;


