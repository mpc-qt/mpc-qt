## Interprocess Communication

### JSON

MPC supports rudimentary control by sending data to its unix socket.  On Linux
this socket is at `/tmp/cmdrkotori.mpc-qt`.  It can be written to from the
commandline with `socat`.

The JSON format is quite simple, a so-called dictionary or key-value map.  The
command is given by the "command" field, like this:

```
{
   "command": "someAction",
   "foo": bar
}
```

The *play* command takes the extra parameter `file` (a string) and processes
it in the same manner as `File -> Open File`.

The *playFiles* command takes the extra parameter `files` (an array) and
optionally `directory` (a string).  It processes the given filenames relative
to the specified directory if provided, and processes it in the same manner as
`File -> Quick Open`.  This command was designed for passing files specified
on the command line through to an already-running process.

The *pause* command pauses the current playback.  If nothing is being played,
nothing happens.

The *unpause* command unpauses the current playback.  If nothing is being
played, nothing happens.

The *start* command requests that the currently active item in the currently
shown playlist is played.  This is not the currently highlighted item in the
playlist, it is the bold aka active item.  To make a item active using the
mouse, double click it.

The *stop* command stops the player.

The *repeat* command repeats the currently played file.  If nothing is being
played, nothing happens.

The *next* command takes an optional parameter `autostart` (a boolean
defaulting to `false`), and proceeds to the next file in the playlist when
playback is active.  If nothing is being played, behavior is defined by the
autostart field: behave like the *start* command when true, move the active
item forwards by one if false.

The *previous* command behaves just like the *next* command, except for
playing/moving to the previous track instead of the next track.

The *togglePlayback* toggles the paused state, and if no file is currently
being played, attempts to start one in the same manner as *start*.


#### Internal Mpv Queries

The ipc interface also provides a mechanism for passing through custom queries
of the internal mpv state, and executing some commands.  These ipc commands
are called *setMpvOption*, *setMpvProperty*, *getMpvProperty*, and
doMpvCommand*.  Note that some mpv properties and commands are filtered
because they are managed internally by mpv or would cause undefined behavior,
and will return an error code of -0xdedbeef.  See the section below.

Each of these commands take the optional parameter `name`, which indicates
what value to query or change, or what command to execute.  Additionally, the
setter commands require the optional parameter `value`, which indicates what
value to pass.

The *doMpvCommand* uses the optional parameter `options`, a list of subsequent
values to pass through to mpv after the command string.  Those used to the
client api will note that this is everything after the first item passed
through `mpv_command`.  So the `options` field can be omitted in some cases.

Final notes:  Because every one of these functions return a value, they have
to block the gui thread, so they should be used sparingly.


#### Return payload

If a ipc command is processed, a key-value map will be returned in JSON format
as follows:

```
{
   "code": status
   "value": value returned if present
}
```

The *code* field shows what sort of action took place.  If `unknown` is
returned, an invalid ipc command was specified.  If `ok` is returned, the
ipc command was executed sucessfully.  If `error` is returned, the ipc command
was executed, but something bad happened.

The *value* field is the value returned by the ipc function if any.  It will
be an mpv error code when something bad happens, and will be nonexistant if an
unknown ipc command was attempted.  It will be null if the ipc did not return
a value, and any other value when the ipc returned something.


### Direct Mpv Access

Sit tight for a bit while the developer does some guru meditation.


### MPRIS

Currently unimplemented.
