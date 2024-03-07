# Jack Sink Wwise Plugin

This plugin exposes a new device to Wwise which allows to connect Wwise to the [Jackaudio](https://jackaudio.org/) Server.
The plugin creates a jack client, registers one output port for each channel of the assigned Wwise bus, and (optionally) connects the output ports of the created client to input ports of a target client.

The automatic port connection can be controlled through the parameters `jcName`, `jcOutPortPrefix`, `jtName`, `jtInPortPrefix`, and `jtAutoConnect` (see plugin documentation for more details).

In order for this plugin to run correctly, the following preconditions must be met:
1. a jackaudio server must be up and running
2. the audio buffer of the jack server must be smaller or equal to the audio buffer of Wwise (preferably equal)

In order for any Wwise parameter changes of the plugin to take any effect in UE4 follow the following steps:
1. make the change in Wwise authoring tool
2. save the change in Wwise authoring tool
3. generate audio data in UE4
4. save generated bank in UE4
5. restart UE4

The here described plugin was tested with
 - Unreal Engine version 4.27.2
 - Wwise Authoring Tool and SDK version 2021.1.10
 - audiojack library version 1.9.21

## Installation

Unfortunately, I was unable to package the plugin for the Wwise Launcher because of the missing macOS platform build (no mac at my disposal at the time of this writing).
The platform macOS must be enabled when installing the Wwise SDK in order for the Wwsie UE4 integration to work, hence, the plugin must also support the platform macOS to be accepted as a valid source in the Wwise Launcher.

As a consequence, the plugin must be compiled (precompiled targets are provided in the `bundles` folder) and installed manually.

### Install Wwise

In order to install Wwise an Audiokinetic account needs to be created.
Once logged in [download](https://www.audiokinetic.com/en/download) the Audiokinetic Launcher.
Start the launcher and install a Wwise version.
This plugin was only tested with version `2021.1.10`, hence, this is the recommended version to choose.
**Make sure to enable macOS, Windows Visual Studio 2017, 2019, and 2022 as these are needed for UE4 integration.**

### Manually Install Precompiled Plugin Bundle to Wwise

In order to install or update the JackSink plugin in Wwise extract all archives form a prebuilt bundle and copy the extracted folders into the Wwise installation folder.
> :warning: WARNING  
> Do **not** copy the file `bundle.json`. Only copy the extracted forlders `SDK` and `Authoring` into the root installation folder of the correct Wwise version.

### Manually Install Precompiled Plugin Bundle to Existing UnrealProject

In order to update the JackSink plugin in an Unreal project where Wwise was already integrated do the following:

* Extract the runtime archive from a prebuilt bundle.
  The runtime archive has a name of the structure `Jack_v<plugin_version>_SDK.Windows_<toolchain>.tar.xz` (e.g. `Jack_v2022.0.2_Buil1_SDK.Windows_vc160.tar.xz`).
* Extract the common archive from a prebuilt bundle.
  The common archive has a name of the structure `Jack_v<plugin_version>_SDK.Common.tar.xz` (e.g. `Jack_v2022.0.2_Buil1_SDK.Common.tar.xz`).
* Copy content the extracted `SDK` folder to `<Unreal project root>/Plugins/Wwise/ThirdParty`.

### Create a New Unreal Project with Wwise Sound Engine

To create a new Unreal project which integrates the Wwsie sound engine including the JackSink plugin do the following.

* Create C++ UE4 project then close UE4
* Start Wwise Launcher
   * navigate to `Unreal`
   * if not already done, download offline integration files (this item can be found in the dropdown menu next to the title "Recent Unreal Engine Projects")
   * choose an unreal project and select "integrate using offline files"

Now the required files have been installed.
The next step consists of registering Wwise and the JackSink plugin in Unreal.
This requires some changes in the build scripts as well as some cpp files.
The following only holds for the Wwise SDK version `2021.1.10.7883`:

* Add `AkAudio` to `PublicDependencyModuleNames` in the Unreal Project build file (`Unreal Project Name.Build.cs`) as described [here](https://www.audiokinetic.com/library/edge/?source=UE4&id=using_cpp.html)
* Modify the `AkAudioDevice` module to incorporate the JackSink:
   * Link the JackSink library by adding `JackSink` to the `AKLibs` list in `AkAudio.Build.cs` (around line 158)
   * Link Jack2 library by adding `libjack` to the list returned by `GetAdditionalWwiseLibs()` in `AkAudio_Windows.Build.cs` (around line 59)
   * Mark the `libjack` DLLs as delay load by adding `PlatformPrefix == "x64" ? "libjack64.dll" : "libjack.dll"` to the list returned by `GetPublicDelayLoadDLLs()` in `AkAudio_Windows.Build.cs` (around line 79)
   * Add the JackSink factory header (`#include <AK/Plugin/JackSinkFactory.h>`) to `AkAudioDevice.cpp` (around line 90)

Compile the UE4 project (this can take several minutes to complete when doing it the first time).
Note that the toolchain version is defined in the function `GetVisualStudioVersion()` in `AkAudio_Windows.Build.cs` (around line 114).
It is dependent on the version of Visual Studio but ignores the configuration setting in Unreal.
I recommend to fix this to `vc160` and install the appropriate toolchain (see [Versions for Building](#versions-for-building)).

Finally, setup the interaction between the Wwise Authoring tool and the Unreal editor.
The following project setting changes are recommended in the Unreal editor:

* Start the Unreal Engine
* Open "Edit -> Project Settings"
   * in "Wwise -> Integration Settigs" enable "Sound Data -> Use Event-Based Packaging"
   * in "Wwise -> User Settings" enable "WAAPI -> Auto Connect to WAAPI"
   * Close the settings window
* Enable "Window -> Waapi Picker" to modify Wwise content directly from UE4

### Configure a Wwise Bus to Play 5th Order Ambisonic Sound through JACK

In order for Wwise to play through the JackSink a corresponding device needs to be created:

* Launch the Wwise authoring tool (this can either be done through the Audiokinetic launcher or by opening the Wwise project created during the Unreal Wwise integration)
* Open the design layout (either by pressing `F5` or through the menu `Layouts->Designer`)
* Open the `Audio` tab in the `Project Explorer`
* Right-click `Audio Devices -> Default Work Unit` and create a new Jack device by selecting `New Child -> Jack`
* Name the new device `Jack` (you can use a different name here but you may need to use this exact same name in the Unreal project settings later on)
* Select `Master-Mixer Hierarchy -> Default Work Unit -> Master Audio Bus` and select `Jack` (or the corresponding name you used when creating the jack device) as `Audio Device`
* Save the changes

### Configure UE4 to Play 5th Order Ambisonic Sound through JACK

You have several options of how to set this up.
The recommended way is to leave the UE4 configuration as is and configure everything through the Wwise authoring tool.
This means that everything played through the authoring tool uses the same audio path as when played through UE4.

Alternatively, it is possible to set the output device and channel configuration within the Unreal project settings (`Edit/Project Settings ...`):

* set the main output of the game engine to the Jack sink (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Audio Device Shareset` to `Jack`).
  Use the exact same name you defined when creating the Jack device in the Wwise authoring tool.
* set the channel count (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Number of Channels` to `36`).
  If this is set to 0 the Jack Sink will terminate immediately after its initialization.
* set the channel config type (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Channel Config Type` to `Ambisonic`).

It is also possible to only set the channel configuration but leave the device selection to the Wwise authoring tool.
This way Wwise will down-mix the Ambisonics signal to the corresponding device or leave it raw when router to Jack.

### Versions for Building

Unfortunately, it is quite a mess to cope with all the different versions of Wwise, UE4, Visual Studio and the corresponding building toolchains and libraries.
Here is what was used to develop this plugin:

- Wwise SDK Version `2021.1.10.7883`.
  ~~At the time of development, a newer 2022 version was available but flagged as beta (in the newer version the [workflow with event-based packaging was marked as deprecated](https://www.audiokinetic.com/library/2022.1.0_7985/?source=UE4&id=using_workflow.html))~~
  The version 2022 is now released as stable version but there are major changes with respect to the unreal integration and I have not yet managed to document the necessary steps for a successful integration.
- Visual Studio 2022 with
   - Desktop Development with C++
   - Game Development with C++
   - To compile the `_vc150` Wwise projects, make sure to install
      - MSVC v141 -VS 2017 C++ x64/x86 build tools
      - C++ ATL for v141 build tools
      - C++ MFC for v141 build tools
   - To compile the `_vc160` Wwise projects, make sure to install
      - MSVC v142 - VS 2019 C++ x64/x86 build tools
      - C++ v14.29 (16.11) ATL for v142 build tools
      - C++ v14.29 (16.11) MFC for v142 build tools
- Unreal Engine v4.27.2

**Important** It is possible to compile all the projects in Visual Studio 2022.
I never converted the project files and left them in their corresponding version (`_vc150`: Visual Studio 2017, `_vc160`: Visual Studio 2019, `_vc170`: Visual Studio 2022) which worked fine for me.

In order for changes in the plugin code to take effect in the Wwise authoring tool
* make sure to close the Wwise Authoring tool (if a build fails with an error of a missing Jack.dll file a likely reason is that a still open Wwise Authoring tool locked the Jack.dll file and the build process was unable to overwrite the old dll with the new one)
* open `Jack_Windows_vc160_static.sln`
* build with `Profile` configuration
* open `WwisePlugin/Jack_Authoring_Windows_vc160.sln`
* build with `Release` configuration

In order for changes in the plugin code to take effect in UE4 
* open `Jack_Windows_vc160_static.sln` (or `_vc150`, depending on the build script `AkAudio_Windows.Build.cs` and/or the Visual Studio version)
* build with `Profile` configuration
* copy the compiled files to the appropriate place in the UE4 project. Refer to the helper file `additional_artifacts_ue4.json` for an example of valid paths. For convenience, it is also possible to change this file to suit your needs and use the Wwise development tools to copy the files (e.g. `python.exe '..\..\Wwise 2021.1.10.7883\Scripts\Build\Plugins\wp.py' package -c -f .\additional_artifacts_ue4.json -v 2022.0.0.1 Windows_vc160`)
* build the UE4 project

## First Sounds in Unreal

To start off it might make sense to have a look at the tutorials provided by Audiokinetic:

* [Introductory Tutorials](https://www.audiokinetic.com/en/library/2021.1.10_7883/?source=UE4&id=gettingstarted.html)
* [Unreal Spatial Audio Tutorials](https://www.audiokinetic.com/en/library/2021.1.10_7883/?source=UE4&id=spatialaudio.html)

To get you started here are a few steps to set up the Wwise project for the [Going From Silence to Sound](https://www.audiokinetic.com/en/library/2021.1.10_7883/?source=UE4&id=gs_silence_to_sound.html) tutorial:

* Download a free audio sample (e.g. [this](https://file-examples.com/wp-content/uploads/2017/11/file_example_WAV_10MG.wav) sample)
* Right-click on `Actor-Mixer Hierarchy -> Default Work Unit` and select `Import Audio Files...` and add the sample file
* Name the new file `Ambient` (you can use a different name here)
* Select the new file and in `General Settings` in `Loop` select `Infinite`
* Right-click the new file and select `New Event -> Play` (use the suggested name)
* Save the changes

This should make the event `Play_Ambient` available to select in the Unreal Engine.

## JackSink Wwise Plugin Development Notes

Some notes on how to create a new Wwise plugin and link the jackaudio library.

### Create a New Wwise Plugin

To create a new Wwise plugin, use the development tools provided by Wwise.
They can be found in a folder of the root installation directory of Wwise (e.g. `C:\Program Files (x86)\Audiokinetic\Wwise 2021.1.9.7847`).
See [the official documentation](https://www.audiokinetic.com/library/edge/?source=SDK&id=effectplugin_tools.html) for more details.
Here is a short version of how to create and configure a plugin for Windows:
* run `python.exe '<WwiseRoot>\Scripts\Build\Plugins\wp.py' new` and follow the prompt.
  Note that the type of the plugin (sink, mixer, source, etc) will be included in the name of the source files. Hence, it is better to avoid it in the name of the plugin.
* Update the file `PremakePlugin.lua` to include and link directory paths, as well as library links needed for the plugin (refer to the Jack plugin as a reference).
* run `python.exe '<WwiseRoot>Scripts\Build\Plugins\wp.py' premake <Platform>` where `<Platform>` stands for the target build platform.
  Information on platforms can be found [here](https://www.audiokinetic.com/library/edge/?source=SDK&id=reference_platform.html).
* To build the plugin, run the command `python.exe '<WwiseRoot>\Scripts\Build\Plugins\wp.py' build` (use `-h` to see the possible options).
  Alternatively, use VisualStudio to build the project (uses the Wwise build scripts underneath).
  The created binaries will be output to the Wwise installation folder.

### Link JACK2 library to Wwise Plugin

* Download and install JACK2
* Copy the `libjack[64].lib` and `libjack[64].dll` files to the plugin folder (e.g. to `thirdParty\jack\x86[_64]`).
* Copy the `libjack` include files to the plugin folder (e.g. to `thirdParty\jack\include`).
* Add the library folder path (e.g. `../thirdParty/jack/%{cfg.architecture}`) and the include folder path (e.g `../thirdParty/jack/include`) as well as the library name (e.g. `libjack`) to the file `PremakePlugin.lua`
* Run the premake developer script (see above).
* When building the different configurations, the plugin library file will automatically be copied to the target SDK location. Note that release builds will be copied to `Profile`. This can be changed in the Solution Properties.