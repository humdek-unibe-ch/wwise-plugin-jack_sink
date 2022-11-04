## Jackaudio Client Output Port Prefix

Specifies the prefix of the output port of the jackaudio client which is created by the plugin.
The prefix will be extended with the channel index starting from 1 (e.g. a prefix `output` for channel 1 will result in a port name `output_1`).
The client name is specified with the parameter `jcName`.
This has no effect if the parameter `jtAutoConnect` is disabled.

**Note**: In order for a change to take effect the plugin needs to be restarted.