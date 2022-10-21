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
    AkReal32* out;
    jack_nframes_t i;
    jack_ringbuffer_data_t vec;
    size_t size = sizeof(AkReal32);

    for (AkUInt32 ch_idx = 0; ch_idx < JACK_SINK_MAX_PORT_COUNT; ch_idx++)
    {
        out = (AkReal32*)jack_port_get_buffer(obj->ports[ch_idx], nframes);
        for (i = 0; i < nframes; i++)
        {
            jack_ringbuffer_get_read_vector(obj->ringbuffer[ch_idx], &vec);
            out[i] = *(AkReal32*)vec.buf;
            jack_ringbuffer_read_advance(obj->ringbuffer[ch_idx], vec.len);

        }
    }

    return 0;
}

AKRESULT JackSink::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkSinkPluginContext* in_pCtx, AK::IAkPluginParam* in_pParams, AkAudioFormat& io_rFormat)
{
    int i;
    char name[100];
    jack_status_t status;
    m_pParams = (JackSinkParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pCtx;

    this->client = jack_client_open("Wwise", JackNullOption, &status);
    if (client == nullptr) {
        return AK_Fail;
    }

    this->buffer_size = this->m_pContext->GlobalContext()->GetMaxBufferLength() * sizeof(AkReal32);
    this->buffer = AK_PLUGIN_ALLOC(in_pAllocator, this->buffer_size);
    for (i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
        sprintf(name, "output_%d", i + 1);
        this->ports[i] = jack_port_register(this->client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        this->ringbuffer[i] = jack_ringbuffer_create(this->buffer_size);
    }

    jack_set_process_callback(this->client, JackSink::ProcessCallback, this);

    jack_activate(this->client);

    return AK_Success;
}

AKRESULT JackSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    jack_deactivate(this->client);

    for (int i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
        jack_port_unregister(this->client, this->ports[i]);
        jack_ringbuffer_free(this->ringbuffer[i]);
    }

    jack_client_close(this->client);
    AK_PLUGIN_FREE(in_pAllocator, this->buffer);
    AK_PLUGIN_DELETE(in_pAllocator, this);

    return AK_Success;
}

AKRESULT JackSink::Reset()
{
    for (int i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
        jack_ringbuffer_reset(this->ringbuffer[i]);
    }
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
    out_uNumFramesNeeded = (AkUInt32)jack_ringbuffer_write_space(this->ringbuffer[0]);
    return AK_Success;
}

void JackSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
    AkUInt32 uNumFrames = in_pInputBuffer->uValidFrames;
    jack_ringbuffer_data_t vec;
    vec.len = sizeof(AkReal32);
    vec.buf = NULL;

    if (uNumFrames > 0)
    {
        AkChannelConfig ch_conf = in_pInputBuffer->GetChannelConfig();
        AkUInt32 uStrideOut = ch_conf.uNumChannels;

        for (AkUInt32 ch_idx = 0; ch_idx < in_pInputBuffer->NumChannels(); ch_idx++) {
            AkReal32* pIn = in_pInputBuffer->GetChannel(ch_idx);
            AkReal32* pEnd = pIn + uNumFrames;
            while (pIn < pEnd)
            {
                vec.buf = (char*)pIn;
                jack_ringbuffer_get_write_vector(this->ringbuffer[ch_idx], &vec);
                jack_ringbuffer_write_advance(this->ringbuffer[ch_idx], vec.len);
                pIn++;
            }
        }
        m_bDataReady = true;
    }
}

void JackSink::OnFrameEnd()
{
    jack_ringbuffer_data_t vec;
    if (!m_bDataReady)
    {
        // Consume was not called for this audio frame, send silence to the output here
        for (int i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
            vec.len = jack_ringbuffer_write_space(this->ringbuffer[i]);
            vec.buf = (char*)this->buffer;
            jack_ringbuffer_get_write_vector(this->ringbuffer[i], &vec);
            jack_ringbuffer_write_advance(this->ringbuffer[i], vec.len);
        }
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
