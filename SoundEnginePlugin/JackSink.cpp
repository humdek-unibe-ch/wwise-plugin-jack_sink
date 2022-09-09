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

#include "jack/jack.h"
#include "JackSink.h"
#include "../JackConfig.h"

#include <AK/AkWwiseSDKVersion.h>

AK::IAkPlugin* CreateJackSink(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, JackSink());
}

AK::IAkPluginParam* CreateJackSinkParams(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, JackSinkParams());
}

AK_IMPLEMENT_PLUGIN_FACTORY(JackSink, AkPluginTypeSink, JackConfig::CompanyID, JackConfig::PluginID)

JackSink::JackSink()
    : m_pParams(nullptr)
    , m_pAllocator(nullptr)
    , m_pContext(nullptr)
    , m_bStarved(false)
    , m_bDataReady(false)
{
}

JackSink::~JackSink()
{
}

int JackSink::ProcessCallback(jack_nframes_t nframes, void* arg)
{
    JackSink* obj = (JackSink*)arg;
    jack_default_audio_sample_t* out1, * out2;
    jack_nframes_t i;

    out1 = (jack_default_audio_sample_t*)jack_port_get_buffer(obj->ports[0], nframes);
    out2 = (jack_default_audio_sample_t*)jack_port_get_buffer(obj->ports[0], nframes);

    for (i = 0; i < nframes; i++)
    {
        out1[i] = 0;
        out2[i] = 0;
    }

    return 0;
}

AKRESULT JackSink::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkSinkPluginContext* in_pCtx, AK::IAkPluginParam* in_pParams, AkAudioFormat& io_rFormat)
{
    jack_status_t status;
    m_pParams = (JackSinkParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pCtx;

    this->client = jack_client_open("Wwise", JackNullOption, &status);
    if (client == nullptr) {
        return AK_Fail;
    }

    this->ports[0] = jack_port_register(this->client, "p0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    this->ports[1] = jack_port_register(this->client, "p1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    jack_set_process_callback(this->client, JackSink::ProcessCallback, this);

    jack_activate(this->client);

    return AK_Success;
}

AKRESULT JackSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    int rc;
    AK_PLUGIN_DELETE(in_pAllocator, this);

    jack_deactivate(this->client);

    jack_port_unregister(this->client, this->ports[0]);
    jack_port_unregister(this->client, this->ports[1]);

    rc = jack_client_close(this->client);
    if (rc < 0) {
        return AK_Fail;
    }

    return AK_Success;
}

AKRESULT JackSink::Reset()
{
    return AK_Success;
}

AKRESULT JackSink::GetPluginInfo(AkPluginInfo& out_rPluginInfo)
{
    out_rPluginInfo.eType = AkPluginTypeSink;
    out_rPluginInfo.bIsInPlace = true;
    out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
    return AK_Success;
}

AKRESULT JackSink::IsDataNeeded(AkUInt32& out_uNumFramesNeeded)
{
    // Set the number of frames needed here
    out_uNumFramesNeeded = m_pContext->GetNumRefillsInVoice();
    return AK_Success;
}

void JackSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
    if (in_pInputBuffer->uValidFrames > 0)
    {
        // Consume input buffer and send it to the output here
        m_bDataReady = true;
    }
}

void JackSink::OnFrameEnd()
{
    if (!m_bDataReady)
    {
        // Consume was not called for this audio frame, send silence to the output here
    }

    m_bDataReady = false;
}

bool JackSink::IsStarved()
{
    return m_bStarved;
}

void JackSink::ResetStarved()
{
    m_bStarved = false;
}
