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

int JackSink::processCallback(jack_nframes_t nframes, void* arg)
{
    JackSink* obj = (JackSink*)arg;
    AkReal32* out;
    jack_nframes_t i;
    jack_ringbuffer_data_t vec;
    size_t size = sizeof(AkReal32);

    for (AkUInt32 ch_idx = 0; ch_idx < obj->channelCount == 0; ch_idx++)
    {
        out = (AkReal32*)jack_port_get_buffer(obj->ports[ch_idx], nframes);
        for (i = 0; i < nframes; i++)
        {
            jack_ringbuffer_get_read_vector(obj->ringbuffer[ch_idx], &vec);
            out[i] = *(AkReal32*)vec.buf;
            jack_ringbuffer_read_advance(obj->ringbuffer[ch_idx], vec.len);
            out[i] = 0;

        }
    }

    return 0;
}

void JackSink::writeLog(const char* fmt, ...) {
    va_list(args);
    va_start(args, fmt);
    SYSTEMTIME lt;

    if (this->fp == NULL) {
        this->fp = fopen("C:\\Users\\moiri\\Documents\\JackSink.log", "a");
    }

    GetLocalTime(&lt);
    fprintf(this->fp, "%04d.%02d.%02d %02d:%02d:%02d.%03d - ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
    vfprintf(this->fp, fmt, args);
    fprintf(this->fp, "\n");
    fflush(this->fp);
}

AKRESULT JackSink::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkSinkPluginContext* in_pCtx, AK::IAkPluginParam* in_pParams, AkAudioFormat& io_rFormat)
{
    jack_port_t* port;
    int rc;
    unsigned int i;
    char port_name[100];
    jack_status_t status;
    m_pParams = (JackSinkParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pCtx;
    AkUniqueID node_uid = in_pCtx->GetAudioNodeID();
    AkUInt32 out_uOutputID;
    AkPluginID out_uDevicePlugin;
    in_pCtx->GetOutputID(out_uOutputID, out_uDevicePlugin);
    this->writeLog("=================================================================================================");
    this->writeLog("IsPrimary: %d", in_pCtx->IsPrimary());
    this->writeLog("node_uid: %d", node_uid);
    this->writeLog("output_id: %d, output_plugin_id: %d", out_uOutputID, out_uDevicePlugin);
    this->channelCount = io_rFormat.GetNumChannels();
    this->writeLog("ChannelCount: %d", this->channelCount);

    if (this->channelCount == 0 || this->channelCount > JACK_SINK_MAX_PORT_COUNT) {
        AKASSERT(!"Invalid channel count");
        return AK_Fail;
    }

    this->writeLog("opening jack client...");
    this->client = jack_client_open("Wwise", JackNullOption, &status);
    if (client == nullptr) {
        return AK_Fail;
    }
    this->writeLog("jack client opened");

    this->writeLog("registering ports...");
    size_t buffer_size = this->m_pContext->GlobalContext()->GetMaxBufferLength() * sizeof(AkReal32);
    for (i = 0; i < this->channelCount; i++) {
        sprintf(port_name, "output_%d", i + 1);
        this->ports[i] = jack_port_register(this->client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        this->ringbuffer[i] = jack_ringbuffer_create(buffer_size);
    }
    this->writeLog("ports registered and buffers created");

    jack_set_process_callback(this->client, JackSink::processCallback, this);
    this->writeLog("callback set");

    jack_activate(this->client);
    this->writeLog("client activated");

    for (i = 0; i < this->channelCount; i++) {
        sprintf(port_name, "SceneRotator:input_%d", i + 1);
        port = jack_port_by_name(this->client, port_name);
        if (port != NULL) {
            rc = jack_connect(this->client, jack_port_name(this->ports[i]), port_name);
            this->writeLog("port connection %s(%s, %d) -> %s(%s, %d): %d", jack_port_name(this->ports[i]), jack_port_type(this->ports[i]), jack_port_flags(this->ports[i]), jack_port_name(port), jack_port_type(port), jack_port_flags(port), rc);
        }
        else
        {
            this->writeLog("cannot connect to port '%s'", port_name);
        }
    }

    return AK_Success;
}

AKRESULT JackSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    jack_deactivate(this->client);
    this->writeLog("client deactivated");

    this->writeLog("unregistering ports...");
    for (unsigned int i = 0; i < this->channelCount; i++) {
        jack_port_unregister(this->client, this->ports[i]);
        jack_ringbuffer_free(this->ringbuffer[i]);
    }
    this->writeLog("ports unregistered");

    jack_client_close(this->client);
    this->writeLog("client closed");

    AK_PLUGIN_DELETE(in_pAllocator, this);

    this->writeLog("*************************************************************************************************");
    fclose(this->fp);

    return AK_Success;
}

AKRESULT JackSink::Reset()
{
    this->writeLog("|||||||||||||||||||||||||");
    for (unsigned int i = 0; i < this->channelCount; i++) {
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
    //out_uNumFramesNeeded = (AkUInt32)jack_ringbuffer_write_space(this->ringbuffer[0]);
    out_uNumFramesNeeded = m_pContext->GetNumRefillsInVoice();
    return AK_Success;
}

void JackSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
    AkUInt32 uNumFrames = in_pInputBuffer->uValidFrames;
    jack_ringbuffer_data_t vec[2];
    size_t size = sizeof(AkReal32);

    if (uNumFrames > 0)
    {
        AkChannelConfig ch_conf = in_pInputBuffer->GetChannelConfig();
        AkUInt32 uStrideOut = ch_conf.uNumChannels;

        for (AkUInt32 ch_idx = 0; ch_idx < in_pInputBuffer->NumChannels(); ch_idx++) {
            AkReal32* pIn = in_pInputBuffer->GetChannel(ch_idx);
            AkReal32* pEnd = pIn + uNumFrames;
            while (pIn < pEnd)
            {
                jack_ringbuffer_get_write_vector(this->ringbuffer[ch_idx], vec);
                if (vec[0].len >= size) {
                    memcpy(vec[0].buf, pIn, size);
                    jack_ringbuffer_write_advance(this->ringbuffer[ch_idx], size);
                }
                else if (vec[1].len >= size) {
                    // buffer size is a multiple of four so data can never be split
                    memcpy(vec[1].buf, pIn, size);
                    jack_ringbuffer_write_advance(this->ringbuffer[ch_idx], size);
                }
                else {
                    // jack is too slow reading data
                }
                pIn++;
            }
        }
        m_bDataReady = true;
    }
}

void JackSink::OnFrameEnd()
{
    jack_ringbuffer_data_t vec[2];
    if (!m_bDataReady)
    {
        // Consume was not called for this audio frame, send silence to the output here
        for (int i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
            jack_ringbuffer_get_write_vector(this->ringbuffer[i], vec);
            if (vec[0].len > 0) {
                memset(vec[0].buf, 0, vec[0].len);
            }
            if (vec[1].len > 0) {
                memset(vec[1].buf, 0, vec[1].len);
            }
            jack_ringbuffer_write_advance(this->ringbuffer[i], vec[0].len + vec[1].len);
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
