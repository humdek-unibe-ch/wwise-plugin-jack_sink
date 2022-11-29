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

#define DEFAULT_BUFFER_COUNT 2

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
    , m_bStarved(false)
    , m_bDataReady(false)
    , channelCount(0)
    , jackNFrames(0)
    , maxNFrames(0)
    , minNFrames(0)
    , wwiseNFrames(0)
    , writeFrameCount(0)
    , readFrameCount(0)
    , rbFrameCount(0)
    , needCount(0)
    , rb(nullptr)
    , client(nullptr)
    , onJackNFramesSet(nullptr)
{
    for (AkUInt32 i = 0; i < DEFAULT_DATA_SIZE; i++) {
        this->silence[i] = 0;
    }
    for (AkUInt32 i = 0; i < JACK_SINK_MAX_PORT_COUNT; i++) {
        this->ports[i] = nullptr;
    }
}

JackSink::~JackSink()
{
}

int JackSink::processCallback(jack_nframes_t nframes, void* arg)
{
    AkUInt32 sampleIdxStart;
    AkUInt32 sampleIdxEnd;
    JackSink* obj = (JackSink*)arg;

    for (AkUInt32 frame_idx = 0; frame_idx < obj->readFrameCount; frame_idx++) {
        sampleIdxStart = frame_idx * obj->minNFrames;
        sampleIdxEnd = (frame_idx + 1) * obj->minNFrames;
        if (nframes < sampleIdxEnd) {
            sampleIdxEnd = nframes;
        }
        for (AkUInt32 ch_idx = 0; ch_idx < obj->channelCount; ch_idx++)
        {
            AkReal32* out = (AkReal32*)jack_port_get_buffer(obj->ports[ch_idx], sampleIdxEnd - sampleIdxStart);
            for (AkUInt32 i = sampleIdxStart; i < sampleIdxEnd; i++)
            {
                if (obj->rb != NULL && jack_ringbuffer_read_space(obj->rb) >= DEFAULT_DATA_SIZE) {
                    jack_ringbuffer_read(obj->rb, (char*)(out + i), DEFAULT_DATA_SIZE);
                }
                else {
                    *(out + i) = 0;
                }
            }
        }
        ::InterlockedExchange(&obj->rbFrameCount, obj->rbFrameCount > 0 ? obj->rbFrameCount - 1 : 0);
    }
#ifdef USE_MY_CUSTOM_DEBUG_LOG_RT
    obj->writeLog("read data");
#endif

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
    char client_name[100];
    jack_status_t status;
    this->m_pParams = (JackSinkParams*)in_pParams;
    AkUInt32 out_uOutputID;
    AkPluginID out_uDevicePlugin;
    in_pCtx->GetOutputID(out_uOutputID, out_uDevicePlugin);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("=================================================================================================");
    this->writeLog("IsPrimary: %d", in_pCtx->IsPrimary());
    this->writeLog("CanPostMonitorData: %d", in_pCtx->CanPostMonitorData());
    this->writeLog("IsRenderingOffline: %d", in_pCtx->GlobalContext()->IsRenderingOffline());
    this->writeLog("node_uid: %d", in_pCtx->GetAudioNodeID());
    this->writeLog("output_id: %d, output_plugin_id: %d", out_uOutputID, out_uDevicePlugin);
    this->writeLog("Params: %s, %s, %d, %s, %s, %d, %d", this->m_pParams->NonRTPC.jcName, this->m_pParams->NonRTPC.jcOutPortPrefix,
        this->m_pParams->NonRTPC.jtAutoConnect, this->m_pParams->NonRTPC.jtName, this->m_pParams->NonRTPC.jtInPortPrefix,
        this->m_pParams->NonRTPC.channelCount, this->m_pParams->NonRTPC.channelType);
#endif

    if (io_rFormat.channelConfig.IsValid())
    {
        this->channelCount = io_rFormat.GetNumChannels();
    }
    else
    {
        this->channelCount = this->m_pParams->NonRTPC.channelCount;
        if (this->m_pParams->NonRTPC.channelType == JACK_SINK_CHANNEL_TYPE_AMBISONICS)
        {
            io_rFormat.channelConfig.SetAmbisonic(this->channelCount);
        }
        else if (this->m_pParams->NonRTPC.channelType == JACK_SINK_CHANNEL_TYPE_ANONYMOUS)
        {
            io_rFormat.channelConfig.SetAnonymous(this->channelCount);
        }
        else
        {
            AKASSERT(!"Unknown channel configuration");
            return AK_UnsupportedChannelConfig;
        }
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("IsChannelConfigValid: %d", io_rFormat.channelConfig.IsValid());
    this->writeLog("ChannelCount: %d", this->channelCount);
#endif

    AKPLATFORM::AkCreateEvent(this->onJackNFramesSet);

    if (this->channelCount == 0 || this->channelCount > JACK_SINK_MAX_PORT_COUNT) {
        AKASSERT(!"Invalid channel count");
        return AK_Fail;
    }

    HMODULE dll = LoadLibrary(L"libjack64.dll");
    if (dll == NULL) {
        AKASSERT(!"No JACKaudio library installed");
        return AK_Fail;
    }

    if (in_pCtx->CanPostMonitorData())
    {
        sprintf(client_name, "%s (Authoring)", this->m_pParams->NonRTPC.jcName);
    }
    else
    {
        sprintf(client_name, "%s", this->m_pParams->NonRTPC.jcName);
    }

    this->client = jack_client_open(client_name, JackNoStartServer, &status);
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
    this->readFrameCount = (AkUInt32)ceil((AkReal32)this->jackNFrames / this->wwiseNFrames);
    this->writeFrameCount = (AkUInt32)ceil((AkReal32)this->wwiseNFrames / this->jackNFrames);
    size_t buffer_size = this->maxNFrames * DEFAULT_DATA_SIZE * DEFAULT_BUFFER_COUNT * this->channelCount;
    this->rb = jack_ringbuffer_create(buffer_size);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
    this->writeLog("buffers created (nframes wwise: %d, nframes jack: %d, read factor: %d, write factor: %d, buffer size: %d)", this->wwiseNFrames, this->jackNFrames, this->readFrameCount, this->writeFrameCount, buffer_size);
#endif
    
    if (this->m_pParams->NonRTPC.jtAutoConnect) {
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
    }

    return AK_Success;
}

AKRESULT JackSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    if (this->client != nullptr) {
        jack_deactivate(this->client);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
        this->writeLog("client deactivated");
#endif
        for (unsigned int i = 0; i < this->channelCount; i++) {
            jack_port_unregister(this->client, this->ports[i]);
        }

        jack_client_close(this->client);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
        this->writeLog("client closed");
#endif
    }


    if (this->rb != nullptr) {
        jack_ringbuffer_free(this->rb);

#ifdef USE_MY_CUSTOM_DEBUG_LOG
        this->writeLog("ports unregistered");
#endif
    }

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

    if (this->rb != nullptr) {
        jack_ringbuffer_reset(this->rb);
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
    if (this->readFrameCount == this->writeFrameCount && this->rbFrameCount == 0) {
        out_uNumFramesNeeded = 1;
    }
    else if (this->readFrameCount < this->writeFrameCount && this->rbFrameCount <= ceil(this->writeFrameCount / 2.0)) {
        out_uNumFramesNeeded = 1;
    }
    else if (this->readFrameCount > this->writeFrameCount && this->rbFrameCount == 0) {
        out_uNumFramesNeeded = this->readFrameCount;
    }
    else {
        out_uNumFramesNeeded = 0;
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG_RT
    this->writeLog("needed (%d): %d", this->rbFrameCount, out_uNumFramesNeeded);
#endif
    return AK_Success;
}

void JackSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
    AkUInt32 uNumFrames = in_pInputBuffer->uValidFrames;
    in_pInputBuffer->ZeroPadToMaxFrames();

    for (AkUInt32 frame_idx = 0; frame_idx < this->writeFrameCount; frame_idx++) {
        for (AkUInt32 ch_idx = 0; ch_idx < in_pInputBuffer->NumChannels(); ch_idx++) {
            AkReal32* pIn = in_pInputBuffer->GetChannel(ch_idx);
            for (AkUInt32 i = frame_idx * this->minNFrames; i < (frame_idx + 1) * this->minNFrames; i++)
            {
                if (jack_ringbuffer_write_space(this->rb) >= DEFAULT_DATA_SIZE) {
                    jack_ringbuffer_write(this->rb, (char*)(pIn + i), DEFAULT_DATA_SIZE);
                }
            }
        }
        ::InterlockedIncrement(&this->rbFrameCount);
    }

#ifdef USE_MY_CUSTOM_DEBUG_LOG_RT
    this->writeLog("write data");
#endif
    this->m_bDataReady = true;
}

void JackSink::OnFrameEnd()
{
    if (!this->m_bDataReady)
    {
        // Consume was not called for this audio frame, send silence to the output here
        for (AkUInt32 ch_idx = 0; ch_idx < this->channelCount; ch_idx++) {
            for (AkUInt32 i = 0; i < this->maxNFrames; i++)
            {
                if (jack_ringbuffer_write_space(this->rb) >= DEFAULT_DATA_SIZE) {
                    jack_ringbuffer_write(this->rb, this->silence, DEFAULT_DATA_SIZE);
                }
            }
        }

#ifdef USE_MY_CUSTOM_DEBUG_LOG_RT
        this->writeLog("write silence");
#endif
        ::InterlockedExchange(&this->rbFrameCount, this->rbFrameCount + this->writeFrameCount);
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
