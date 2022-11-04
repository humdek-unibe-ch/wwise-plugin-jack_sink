## Auto-Connect Jackaudio Client to Target Client

I enabled the plugin tries to automatically connect the output ports of created jackaudio client to ports of a target client.
The name matching is specified by the parameters `jcName`, `jcOutPortPrefix` for the client created by the plugin and by the parameters `jtName` and `jtInPortPrefix` for the target client to connect to.
If disabled, no automatic connection is attmepted (the preferred method for automatic port connections is the patchbay of the QJackCtrl software).

**Note**: In order for a change to take effect the plugin needs to be restarted.