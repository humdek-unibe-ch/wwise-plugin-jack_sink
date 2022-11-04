## Jackaudio Target Client Input Port Prefix

Specifies the prefix of the input port of the jackaudio client to which to connect automatically.
The prefix will be extended with the channel index starting from 1 (e.g. a prefix `input` for channel 1 will result in a port name `input_1`).
The target client name is specified with the parameter `jtName`.
This has no effect if the parameter `jtAutoConnect` is disabled.

**Note**: In order for a change to take effect the plugin needs to be restarted.