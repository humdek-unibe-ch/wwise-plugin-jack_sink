/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the
"Apache License"); you may not use this file except in compliance with the
Apache License. You may obtain a copy of the Apache License at
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Copyright (c) 2021 Audiokinetic Inc.
*******************************************************************************/

#include "JackSinkParams.h"

#include <AK/Tools/Common/AkBankReadHelpers.h>

JackSinkParams::JackSinkParams()
{
}

JackSinkParams::~JackSinkParams()
{
}

JackSinkParams::JackSinkParams(const JackSinkParams& in_rParams)
{
    RTPC = in_rParams.RTPC;
    NonRTPC = in_rParams.NonRTPC;
	m_paramChangeHandler.SetAllParamChanges();
}

AK::IAkPluginParam* JackSinkParams::Clone(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, JackSinkParams(*this));
}

AKRESULT JackSinkParams::Init(AK::IAkPluginMemAlloc* in_pAllocator, const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    if (in_ulBlockSize == 0)
    {
        // Initialize default parameters here
        sprintf(NonRTPC.jcName, "WwiseJackSink");
        sprintf(NonRTPC.jcOutPortPrefix, "output");
        sprintf(NonRTPC.jtName, "SceneRotator");
        sprintf(NonRTPC.jtInPortPrefix, "input");
        NonRTPC.jtAutoConnect = false;
        NonRTPC.channelCount = JACK_SINK_MAX_PORT_COUNT;
        NonRTPC.channelType = JACK_SINK_CHANNEL_TYPE_AMBISONICS;
        m_paramChangeHandler.SetAllParamChanges();
        return AK_Success;
    }

    return SetParamsBlock(in_pParamsBlock, in_ulBlockSize);
}

AKRESULT JackSinkParams::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    AK_PLUGIN_DELETE(in_pAllocator, this);
    return AK_Success;
}

AKRESULT JackSinkParams::SetParamsBlock(const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    AkUInt32 len;
    AKRESULT eResult = AK_Success;
    AkUInt8* pParamsBlock = (AkUInt8*)in_pParamsBlock;

    // Read bank data here
    sprintf(NonRTPC.jcName, READBANKSTRING(pParamsBlock, in_ulBlockSize, len));
    sprintf(NonRTPC.jcOutPortPrefix, READBANKSTRING(pParamsBlock, in_ulBlockSize, len));
    sprintf(NonRTPC.jtName, READBANKSTRING(pParamsBlock, in_ulBlockSize, len));
    sprintf(NonRTPC.jtInPortPrefix, READBANKSTRING(pParamsBlock, in_ulBlockSize, len));
    NonRTPC.jtAutoConnect = READBANKDATA(bool, pParamsBlock, in_ulBlockSize);
    NonRTPC.channelCount = READBANKDATA(AkUInt32, pParamsBlock, in_ulBlockSize);
    NonRTPC.channelType = READBANKDATA(AkInt32, pParamsBlock, in_ulBlockSize);
    CHECKBANKDATASIZE(in_ulBlockSize, eResult);
    m_paramChangeHandler.SetAllParamChanges();

    return eResult;
}

AKRESULT JackSinkParams::SetParam(AkPluginParamID in_paramID, const void* in_pValue, AkUInt32 in_ulParamSize)
{
    AKRESULT eResult = AK_Success;

    // Handle parameter change here
    switch (in_paramID)
    {
    case PARAM_JC_NAME_ID:
        sprintf(NonRTPC.jcName, (const char*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_JC_NAME_ID);
        break;
    case PARAM_JC_OUT_PORT_PREFIX_ID:
        sprintf(NonRTPC.jcOutPortPrefix, (const char*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_JC_OUT_PORT_PREFIX_ID);
        break;
    case PARAM_JT_AUTO_CONNECT_ID:
        m_paramChangeHandler.SetParamChange(PARAM_JT_AUTO_CONNECT_ID);
        NonRTPC.jtAutoConnect = *reinterpret_cast<const bool*>(in_pValue);
        break;
    case PARAM_JT_NAME_ID:
        sprintf(NonRTPC.jtName, (const char*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_JT_NAME_ID);
        break;
    case PARAM_JT_IN_PORT_PREFIX_ID:
        sprintf(NonRTPC.jtInPortPrefix, (const char*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_JT_IN_PORT_PREFIX_ID);
        break;
    case PARAM_CHANNEL_COUNT_ID:
        m_paramChangeHandler.SetParamChange(PARAM_CHANNEL_COUNT_ID);
        NonRTPC.channelCount = *reinterpret_cast<const AkUInt32*>(in_pValue);
        break;
    case PARAM_CHANNEL_TYPE_ID:
        m_paramChangeHandler.SetParamChange(PARAM_CHANNEL_TYPE_ID);
        NonRTPC.channelType = *reinterpret_cast<const AkInt32*>(in_pValue);
        break;
    default:
        eResult = AK_InvalidParameter;
        break;
    }

    return eResult;
}
