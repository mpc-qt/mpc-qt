## Interprocess Communication

### JSON

MPC supports rudimentary control by sending data to its unix socket.  On Linux
this socket is at `/tmp/cmdrkotori.mpc-qt`.  It can be written to from the
commandline with `socat`.  If processed, the text `ACK` will be returned.

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

The *start* command requests that the currently selected item in the currently
shown playlist is played.  Acts in the same way as pressing the Enter key when
the playlist window has focus.

The *stop* command stops the player.

The *repeat* command repeats the currently played file.  If nothing is being
played, nothing happens.

The *next* command takes an optional parameter `autostart` (a boolean
defaulting to `false`), and proceeds to the next file in the playlist when
playback is active.  If nothing is being played, behavior is defined by the
autostart field: behave like the *start* command when true, move the playlist
selection forwards by one if false.

The *previous* command behaves just like the *next* command, except for
playing/moving to the previous track instead of the next track.

The *togglePlayback* toggles the paused state, and if no file is currently
being played, attempts to start one in the same manner as *start*.

### MPRIS

Currently unimplemented.
