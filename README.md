# Jack Sink Wwise Plugin

This plugin exposes a new device to Wwise which allows to connect Wwise the [Jackaudio](https://jackaudio.org/) Server.
The plugin creates a jack client, registers one output port for each channel of the assigned Wwise bus and (optionally) connects the output ports of the created client to input ports of a target client.

The automatic port connection can be controlled through the parameters `jcName`, `jcOutPortPrefix`, `jtName`, `jtInPortPrefix`, and `jtAutoConnect` (see plugin documentation for more details).

In order for this plugin to run correctly to following preconditions must be met:
1. a jackaudio server must be up and running
2. the audio buffer of the jack server must be smaller or equal to the audio buffer of Wwise (preferrably equal)

In order for any Wwise parameter changes of the plugin to take any effect in UE4 follow to following steps:
1. make the change in Wwise auditing tool
2. save the change in Wwise auditing tool
3. generate audio data in UE4
4. save generated bank in UE4
5. restart UE4

The here described plugin was tested with
 - Unreal Engine version 4.27.2
 - Wwise Authoring Tool and SDK version 2021.1.10
 - audiojack library version 1.9.21

## Installation

Unfortunately I was not able to package the plugin for the Wwise Launcher because of the missing MacOs platform build (no mac at my disposal at the time of this writing).
The platform MacOs must be enabled when installing the Wwise SDK in order for the Wwsie UE4 integration to work, hence, the plugin must also support the platform MacOs to be accepted as a valid source in the Wwise Launcher.

As a consequence, the plugin must be compiled (precompiled targets are provided in the dist folder) and installed manually.

### Setup Wwise with UE4

* Create UE4 project then close UE4
* Start Wwise Launcher
   * install a Wwise version (make sure to enable MacOs, Windows Visual Studio 2017, 2019, and 2022 as these are needed for UE4 integration)
   * navigate to `Unreal`
   * download offline integration files (this item can be found in the dropdown menu next to the title "Recent Unreal Engine Projects")
   * choose a unreal project and select "integrate using offline files"
* Link the AkModule to the UE4 game as described [here](https://www.audiokinetic.com/library/edge/?source=UE4&id=using_cpp.html)
* Lauch UE4 project
   * Open "Edit -> Project Settings"
      * in "Wwise -> Integration Settigs" enable "Sound Data -> Use Event-Based Packaging"
      * in "Wwise -> User Settings" enable "WAAPI -> Auto Connect to WAAPI"
      * Close the settings window
   * Enable "Window -> Waapi Picker" to modify Wwise content directly from UE4
* Launch Wwise project

### Register the Jack Wwise Plugin in UE4

Modify the `AkAudioDevice` module to incorporate the JackSink:

* Add the JackSink factory header (`#include <AK/Plugin/JackSinkFactory.h>`) to `AkAudioDevice.cpp` (around line 90)
* Link the JackSink library by adding it to the `AKLibs` list in `AkAudio.Build.cs` (around line 158)
* Link the Jack library by adding the path to the library with `PublicAdditionalLibraries.Add("C:/Program Files/JACK2/lib/libjack64.lib");` in `AkAudio.Build.cs` (around line 309).
  It might be a good option to add the file `libjack64.lib` to the Wwise SDK folder with the [packaging tool](https://www.audiokinetic.com/library/2022.1.0_7985/?source=SDK&id=effectplugin_tools_packaging.html) and the add the library the same way as the `JackSink` library.
* In the Unreal project settings (`Edit/Project Settings ...`)
   - set the main output of the game engine to the Jack sink (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Audio Device Shareset` to `Jack`).
   - set the channel count (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Number of Channels` to `36`). If this is set to 0 the Jack Sink will terminate immediately after its initialisation.
   - set the channel config type (e.g. set `Wwise/Windows/Common Settings/Main Output Settings/Channel Config Type` to `Ambisonic`)).

### Versions for Building

Unfortunately, it is quite a mess to cope with all the different versions of Wwise, UE4, Visual Studio and the corresponding building toolchains and libraries.
Here is what was used to develop this plugin:

- Wwise SDK Version `2021.1.10.7883`
  At the time of development a newer 2022 version was available but flagged as beta (in the newer version the [workflow with event-based packaging was marked as deprecated](https://www.audiokinetic.com/library/2022.1.0_7985/?source=UE4&id=using_workflow.html))
- Visual Studio 2022 with
   - Desktop Development with C++
   - Game Development with C++
   - To compile the `_vc150` Wwise projects (required for UE4 v4.27) make sure to install
      - MSVC v141 -VS 2017 C++ x64/x86 build tools
      - C++ ATL for v141 build tools
      - C++ MFC for v141 build tools
   - To compile the `_vc160` Wwise projects (required for Wwise Authoring Tool 2021.1.10) make sure to install
      - MSVC v142 - VS 2019 C++ x64/x86 build tools
      - C++ v14.29 (16.11) ATL for v142 build tools
      - C++ v14.29 (16.11) MFC for v142 build tools
- Unreal Engine v4.27.2

**Important** It is possible to compile all the projects in Visual Studio 2022.
I never converted the project files and left them in their corresponding version (`_vc150`: Visual Studio 2017, `_vc160`: Visual Studio 2019), something I would recommend to preserve the manual changes that were necessery in order to correctly link the jackaudio library.

In order for changes in the plugin code to take effect in the Wwise authoring tool
* open `Jack_Windows_vc160_static.sln`
* build with `Profile` configuratrion
* open `WwisePlugin/Jack_Authoring_Windows_vc160.sln`
* build with `Release` configuration

In order for changes in the plugin code to take effect in UE4 
* open `Jack_Windows_vc150_static.sln`
* build with `Profile` configuratrion
* copy the compiled files to the appropriate place in the UE4 project. Refer to the helper file `additional_artifacts_ue4.json` for an example of valid paths. For convenience it is also possible to change this file to suit your needs and use the Wwise development tools to copy the files (e.g. `python.exe '..\..\Wwise 2021.1.10.7883\Scripts\Build\Plugins\wp.py' package -c -f .\additional_artifacts_ue4.json -v 2022.0.0.1 Windows_vc150`)
* build the UE4 project

## JackSink Wwise Plugin Development Notes

Some notes on how to create a new Wwise plugin and link the jackaudio library.

### Create a New Wwise Plugin

To create a new Wwise plugin use the development tools provided by Wwise.
They can be found in a folder of the root installation directory of Wwise (e.g. `C:\Program Files (x86)\Audiokinetic\Wwise 2021.1.9.7847`).
See [the official documentation](https://www.audiokinetic.com/library/edge/?source=SDK&id=effectplugin_tools.html) for more details.
Here a short version of how to create and configure a plugin for Windows:
* run `python.exe '<WwiseRoot>\Scripts\Build\Plugins\wp.py' new` and follow the prompt.
  Note that the type of the plugin (sink, mixer, source, etc) will be included in the name of the source files. Hence, it is better to avoid it in the name of the plugin.
* Update the file `PremakePlugin.lua` to include include and link directory paths as well as library links needed for the plugin (refer to the Jack plugin as a reference).
* run `python.exe '<WwiseRoot>Scripts\Build\Plugins\wp.py' premake <Platform>` where `<Platform>` stands for the target build platform.
  Information on platforms can be found [here](https://www.audiokinetic.com/library/edge/?source=SDK&id=reference_platform.html).
* To build the plugin run the command `python.exe '<WwiseRoot>\Scripts\Build\Plugins\wp.py' build` (use `-h` to see the possible options).
  Alternatively, use VisualStudio to build the project (uses the Wwise build scripts underneeth).
  The created binaries will be output to the Wwise installation folder.

### Link JACK2 library to Wwise Plugin

* Donwload and install JACK2
* Add the library folder path (e.g. `C:/Program Files/JACK2/lib32`) and the include folder path (e.g `C:/Program Files/JACK2/include`) as well as the library name (e.g. `libjack`) to the file `PremakePlugin.lua`
* Run the premake developper script (see above). Unfortunately, it is not possible to setup different library links for x64 and win32 versions. Hence, a manual editing of the libjack links is necessary. For all Win32 build configurations, change
   * the library path for the `Jack` project in `Properties->Linker->General->Additional Dependencies` from `(...)/lib` to `(...)/lib32`.
   * the library name for the `Jack` project in `Properties->Linker->Input->Additional Library Directories` from `libjack64.lib` to `libjack.lib`.
   * the library path for the `JackSink` project in `Properties->Librarian->General->Additional Dependencies` from `(...)/lib` to `(...)/lib32`.
   * the library name for the `Jack` project in `Properties->Librarian->General->Additional Library Directories` from `libjack64.lib` to `libjack.lib`.
* When building the differnt configurations, the plugin library file will automatically be copied to the target SDK location. Note that release builds will be copied to `Profile`. This can be changed in the Solution Properties.