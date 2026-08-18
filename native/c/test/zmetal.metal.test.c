/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

#include "zmetal.metal.test.h"
#include "zmetal.h"

#pragma prolog(ZMTLLOAD, " ZWEPROLG NEWDSA=(YES,4) ")
#pragma epilog(ZMTLLOAD, " ZWEEPILG ")
void *ZMTLLOAD(const char *name)
{
    void *ep = load_module(name);
    return ep;
}

#pragma prolog(ZMTLDEL, " ZWEPROLG NEWDSA=(YES,4) ")
#pragma epilog(ZMTLDEL, " ZWEEPILG ")
int ZMTLDEL(const char *name)
{
    return delete_module(name);
}

// returns 1 if the IEAVJAOF service (used by auth_off) is available via ECVTJAOF
#pragma prolog(ZMTLJAOF, " ZWEPROLG NEWDSA=(YES,1) ")
#pragma epilog(ZMTLJAOF, " ZWEEPILG ")
int ZMTLJAOF()
{
    PSA *psa = (PSA *)0;
    struct cvt *PTR32 cvt_a = (struct cvt * PTR32) psa->flccvt;
    struct ecvt *PTR32 ecvt_a = (struct ecvt * PTR32) cvt_a->cvtecvt;
    return 0 != ecvt_a->ecvtjaof ? 1 : 0;
}

// returns 1 if the caller is running in problem state
#pragma prolog(ZMTLPST, " ZWEPROLG NEWDSA=(YES,1) ")
#pragma epilog(ZMTLPST, " ZWEEPILG ")
int ZMTLPST()
{
    PSW psw = {0};
    get_psw(&psw);
    return psw.data.bits.p ? 1 : 0;
}

// returns the TESTAUTH result (0 = authorized)
#pragma prolog(ZMTLTAUT, " ZWEPROLG NEWDSA=(YES,1) ")
#pragma epilog(ZMTLTAUT, " ZWEEPILG ")
int ZMTLTAUT()
{
    return test_auth();
}
