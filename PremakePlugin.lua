--[[----------------------------------------------------------------------------
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
------------------------------------------------------------------------------]]

if not _AK_PREMAKE then
    error('You must use the custom Premake5 scripts by adding the following parameter: --scripts="Scripts\\Premake"', 1)
end

local Plugin = {}
Plugin.name = "Jack"
Plugin.factoryheader = "../SoundEnginePlugin/JackSinkFactory.h"
Plugin.sdk = {}
Plugin.sdk.static = {}
Plugin.sdk.shared = {}
Plugin.authoring = {}

-- SDK STATIC PLUGIN SECTION
Plugin.sdk.static.includedirs = -- https://github.com/premake/premake-core/wiki/includedirs
{
    "C:/Program Files/JACK2/include"
}
Plugin.sdk.static.files = -- https://github.com/premake/premake-core/wiki/files
{
    "**.cpp",
    "**.h",
    "**.hpp",
    "**.c",
}
Plugin.sdk.static.excludes = -- https://github.com/premake/premake-core/wiki/removefiles
{
    "JackSinkShared.cpp"
}
Plugin.sdk.static.links = -- https://github.com/premake/premake-core/wiki/links
{
    "libjack64"
}
Plugin.sdk.static.libsuffix = "Sink"
Plugin.sdk.static.libdirs = -- https://github.com/premake/premake-core/wiki/libdirs
{
    "C:/Program Files/JACK2/lib"
}
Plugin.sdk.static.defines = -- https://github.com/premake/premake-core/wiki/defines
{
}

-- SDK SHARED PLUGIN SECTION
Plugin.sdk.shared.includedirs =
{
    "C:/Program Files/JACK2/include"
}
Plugin.sdk.shared.files =
{
    "JackSinkShared.cpp",
    "JackSinkFactory.h",
}
Plugin.sdk.shared.excludes =
{
}
Plugin.sdk.shared.links =
{
    "libjack64"
}
Plugin.sdk.shared.libdirs =
{
    "C:/Program Files/JACK2/lib"
}
Plugin.sdk.shared.defines =
{
}

-- AUTHORING PLUGIN SECTION
Plugin.authoring.includedirs =
{
    "C:/Program Files/JACK2/include"
}
Plugin.authoring.files =
{
    "**.cpp",
    "**.h",
    "**.hpp",
    "**.c",
    "Jack.def",
    "Jack.xml",
    "*.rc",
}
Plugin.authoring.excludes =
{
}
Plugin.authoring.links =
{
    "libjack64"
}
Plugin.authoring.libdirs =
{
    "C:/Program Files/JACK2/lib"
}
Plugin.authoring.defines =
{
}

return Plugin
