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
#include <math.h>

#include <AK/AkWwiseSDKVersion.h>

#define DEFAULT_DATA_SIZE sizeof(AkReal32)
#define DEFAULT_BUFFER_COUNT 1

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
    , nFramesWritten(0)
    , rb(nullptr)
    , client(nullptr)
{
}

JackSink::~JackSink()
{
}

int JackSink::processCallback(jack_nframes_t nframes, void* arg)
{
    JackSink* obj = (JackSink*)arg;

    for (AkUInt32 ch_idx = 0; ch_idx < obj->channelCount; ch_idx++)
    {
        AkReal32* out = (AkReal32*)jack_port_get_buffer(obj->ports[ch_idx], nframes);
        for (AkUInt32 i = 0; i < obj->minNFrames; i++)
        {
            if (obj->rb != NULL && jack_ringbuffer_read_space(obj->rb) >= DEFAULT_DATA_SIZE) {
                jack_ringbuffer_read(obj->rb, (char*)(out + i), DEFAULT_DATA_SIZE);
            }
            else {
                *out = 0;
            }
        }
    }
    if (nframes > obj->minNFrames) {
        for (AkUInt32 ch_idx = 0; ch_idx < obj->channelCount; ch_idx++)
        {
            AkReal32* out = (AkReal32*)jack_port_get_buffer(obj->ports[ch_idx], nframes);
            for (AkUInt32 i = obj->minNFrames; i < obj->maxNFrames; i++)
            {
                if (obj->rb != NULL && jack_ringbuffer_read_space(obj->rb) >= DEFAULT_DATA_SIZE) {
                    jack_ringbuffer_read(obj->rb, (char*)(out + i), DEFAULT_DATA_SIZE);
                }
                else {
                    *(out + i) = 0;
                }
            }
        }
    }

    ::InterlockedExchange(&obj->nFramesWritten, obj->nFramesWritten >= nframes ? obj->nFramesWritten - nframes : 0);

    return 0;
}

int JackSink::setBufferSizeCallback(jack_nframes_t nframes, void* arg)
{
    JackSink* obj = (JackSink*)arg;
    obj->jackNFrames = nframes;

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    obj->writeLog("jack frames %d", obj->jackNFrames);
#endif

    AKPLATFORM::AkSignalEvent(obj->onJackNFramesSet);

    return 0;
}

#ifdef USE_MY_CUSTOM_DEBUG_LOG
void JackSink::writeLog(const char* fmt, ...) {
    va_list(args);
    va_start(args, fmt);
    SYSTEMTIME lt;

    if (this->fp == NULL) {
        this->fp = fopen("C:\\Users\\moiri\\Documents\\JackSink.log", "w");
    }

    GetLocalTime(&lt);
    fprintf(this->fp, "%04d.%02d.%02d %02d:%02d:%02d.%03d - ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
    vfprintf(this->fp, fmt, args);
    fprintf(this->fp, "\n");
    fflush(this->fp);
}
#endif

AKRESULT JackSink::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkSinkPluginContext* in_pCtx, AK::IAkPluginParam* in_pParams, AkAudioFormat& io_rFormat)
{
    jack_port_t* port;
    int rc;
    unsigned int i;
    char port_name[100];
    jack_status_t status;
    this->m_pParams = (JackSinkParams*)in_pParams;
    this->m_pAllocator = in_pAllocator;
    this->m_pContext = in_pCtx;
    AkUniqueID node_uid = in_pCtx->GetAudioNodeID();
    AkUInt32 out_uOutputID;
    AkPluginID out_uDevicePlugin;
    in_pCtx->GetOutputID(out_uOutputID, out_uDevicePlugin);

    this->channelCount = io_rFormat.GetNumChannels();
    AKPLATFORM::AkCreateEvent(this->onJackNFramesSet);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("=================================================================================================");
    this->writeLog("IsPrimary: %d", in_pCtx->IsPrimary());
    this->writeLog("node_uid: %d", node_uid);
    this->writeLog("output_id: %d, output_plugin_id: %d", out_uOutputID, out_uDevicePlugin);
    this->writeLog("ChannelCount: %d", this->channelCount);
    this->writeLog("Params: %s, %s, %s, %s", this->m_pParams->NonRTPC.jcName, this->m_pParams->NonRTPC.jcOutPortPrefix, this->m_pParams->NonRTPC.jtName, this->m_pParams->NonRTPC.jtInPortPrefix);
#endif

    if (this->channelCount == 0 || this->channelCount > JACK_SINK_MAX_PORT_COUNT) {
        AKASSERT(!"Invalid channel count");
        return AK_Fail;
    }

    this->client = jack_client_open(this->m_pParams->NonRTPC.jcName, JackNullOption, &status);
    if (client == nullptr) {
        return AK_Fail;
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("jack client opened");
#endif

    for (i = 0; i < this->channelCount; i++) {
        sprintf(port_name, "%s_%d", m_pParams->NonRTPC.jcOutPortPrefix, i + 1);
        this->ports[i] = jack_port_register(this->client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("ports registered");
#endif

    jack_set_process_callback(this->client, JackSink::processCallback, this);
    jack_set_buffer_size_callback(this->client, JackSink::setBufferSizeCallback, this);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("callback set");
#endif

    jack_activate(this->client);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("client activated");
#endif

    AKPLATFORM::AkWaitForEvent(this->onJackNFramesSet);
    this->wwiseNFrames = in_pCtx->GlobalContext()->GetMaxBufferLength();
    this->maxNFrames = this->wwiseNFrames > this->jackNFrames ? this->wwiseNFrames : this->jackNFrames;
    this->minNFrames = this->wwiseNFrames < this->jackNFrames ? this->wwiseNFrames : this->jackNFrames;
    size_t buffer_size = this->maxNFrames * DEFAULT_DATA_SIZE * DEFAULT_BUFFER_COUNT * this->channelCount;
    this->rb = jack_ringbuffer_create(buffer_size);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("buffers created (nframes wwise: %d, nframes jack: %d, buffer size: %d)", this->wwiseNFrames, this->jackNFrames, buffer_size);
#endif

    for (i = 0; i < this->channelCount; i++) {
        sprintf(port_name, "%s:%s_%d", this->m_pParams->NonRTPC.jtName, this->m_pParams->NonRTPC.jtInPortPrefix, i + 1);
        port = jack_port_by_name(this->client, port_name);
        if (port != NULL) {
            rc = jack_connect(this->client, jack_port_name(this->ports[i]), port_name);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
            this->writeLog("port connection %s(%s, %d) -> %s(%s, %d): %d", jack_port_name(this->ports[i]), jack_port_type(this->ports[i]), jack_port_flags(this->ports[i]), jack_port_name(port), jack_port_type(port), jack_port_flags(port), rc);
#endif
        }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
        else
        {
            this->writeLog("cannot connect to port '%s'", port_name);
        }
#endif
    }

    return AK_Success;
}

AKRESULT JackSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    if (this->client != nullptr) {
        jack_deactivate(this->client);
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("client deactivated");
#endif

    for (unsigned int i = 0; i < this->channelCount; i++) {
        jack_port_unregister(this->client, this->ports[i]);
    }

    if (this->rb != nullptr) {
        jack_ringbuffer_free(this->rb);
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("ports unregistered");
#endif

    jack_client_close(this->client);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("client closed");
#endif

    AKPLATFORM::AkDestroyEvent(this->onJackNFramesSet);
    AK_PLUGIN_DELETE(in_pAllocator, this);


#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("*************************************************************************************************");
    fclose(this->fp);
#endif

    return AK_Success;
}

AKRESULT JackSink::Reset()
{

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("|||||||||||||||||||||||||");
#endif

    jack_ringbuffer_reset(this->rb);
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
    out_uNumFramesNeeded = DEFAULT_BUFFER_COUNT - this->nFramesWritten / this->wwiseNFrames;
    return AK_Success;
}

void JackSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
    AkUInt32 uNumFrames = in_pInputBuffer->uValidFrames;

    if (uNumFrames > 0)
    {
        for (AkUInt32 ch_idx = 0; ch_idx < in_pInputBuffer->NumChannels(); ch_idx++) {
            AkReal32* pIn = in_pInputBuffer->GetChannel(ch_idx);
            for (AkUInt32 i = 0; i < this->minNFrames; i++)
            {
                if (jack_ringbuffer_write_space(this->rb) >= DEFAULT_DATA_SIZE) {
                    jack_ringbuffer_write(this->rb, (char*)(pIn + i), DEFAULT_DATA_SIZE);
                }
            }
        }
        if (uNumFrames > this->minNFrames) {
            for (AkUInt32 ch_idx = 0; ch_idx < in_pInputBuffer->NumChannels(); ch_idx++) {
                AkReal32* pIn = in_pInputBuffer->GetChannel(ch_idx);
                for (AkUInt32 i = this->minNFrames; i < this->maxNFrames; i++)
                {
                    if (jack_ringbuffer_write_space(this->rb) >= DEFAULT_DATA_SIZE) {
                        jack_ringbuffer_write(this->rb, (char*)(pIn + i), DEFAULT_DATA_SIZE);
                    }
                }
            }
        }
        
        ::InterlockedExchange(&this->nFramesWritten, this->nFramesWritten + uNumFrames);
        this->m_bDataReady = true;
    }
}

void JackSink::OnFrameEnd()
{
    if (!this->m_bDataReady)
    {
        // Consume was not called for this audio frame, send silence to the output here
        ::InterlockedExchange(&this->nFramesWritten, this->nFramesWritten + this->wwiseNFrames);
    }

    this->m_bDataReady = false;
}

bool JackSink::IsStarved()
{
    return this->m_bStarved;
}

void JackSink::ResetStarved()
{
    this->m_bStarved = false;
}
