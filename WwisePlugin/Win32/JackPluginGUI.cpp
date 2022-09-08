
#include "JackPluginGUI.h"

JackPluginGUI::JackPluginGUI()
{
}

ADD_AUDIOPLUGIN_CLASS_TO_CONTAINER(
    Jack,            // Name of the plug-in container for this shared library
    JackPluginGUI,   // Authoring plug-in class to add to the plug-in container
    JackSink         // Corresponding Sound Engine plug-in class
);
