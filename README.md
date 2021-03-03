# DYNAMIC LOADING SHENANIGANS

This demonstrates an advanced dynamic loading technique.

## Concept

It is a common pattern to load external "plugins" in the form of dynamic modules into a host application using the `dlopen` family of functions.
When it is desired to have those plugins trigger functionality in the host, this can be accomplished using callbacks, i.e. passing pointers to host functions as arguments to plugin functions.
In this pattern, the signatures of these callbacks often contain a "context object", typically of type void pointer, for which a value is provided at the time of callback registration, and which is passed back to the callback unread/unmodified by the plugin.
In this way, the same host function can be used in multiple contexts, and the function can distinguish which context it was invoked in by examining this context value (the structure of which is, after all, defined by the host).
This feature is especially valuable when multiple instances of the same plugin are desirable.

## Problem

Unfortunately, some libraries do not provide such a context facility, making it difficult to re-use callbacks for different contexts/instances.

* The problem of same-module, multiple-context callbacks can usually be solved by duplicating functions, if the contexts are deterministic. If callbacks are registered per object of some kind, hopefully they already pass a reference to their objects.
* If the architecture is known, per-instances copies of the callback could be dynamically created by copying machine code, but this is very platform specific, rather involved, and potentially dangerous because it involves writeable executable memory
* If the callbacks are only called locally from within a single host call (e.g. in a queue like model, or a `PluginTriggerCallbacks()`-style function), global variables can be set before this to pass the context that way.

The problem of distinguishing instances remains.

## Solution

The return address of the callback will necessarily be inside the plugin that called it; it can be retrieved using the GCC intrinsic function `__builtin_return_address`.
This is already sufficient for using the same callback from different plugins, but for multiple instances of the same plugin, `dlopen` will usually re-use the same code segments and therefore provide the same addresses.
Luckily, this optimization can be disabled ising `dlmopen` with `LM_ID_NEWLM`, leading to better isolation of the plugins.
A known code address can be mapped to the module that loaded it using `dlinfo`, completing the mapping.

## Caveats

The `LM_ID_NEWLM` flag loads the plugin in a completely empty link map, which does not include *any* modules and can therefore not provide any of the symbols to the plugin that would usually provided (e.g. the standard library, or the symbols of the host application itself).
External libraries like the `libc` can be loaded manually, but it seems like sharing the host applications symbols is not possible.
If the host library does not use global variables, it may be possible to at least use its functions by loading a copy of the host application using `dlmopem`.
