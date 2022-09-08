#pragma once

#include "../JackPlugin.h"

class JackPluginGUI final
	: public AK::Wwise::Plugin::PluginMFCWindows<>
	, public AK::Wwise::Plugin::GUIWindows
{
public:
	JackPluginGUI();

};
