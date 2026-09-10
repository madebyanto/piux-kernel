# pWM Commands

## What is pWM?

pWM is Piux's text-based window manager. It is started after login and provides a tiled terminal environment inspired by tiling window managers such as [RatPoison](https://www.nongnu.org/ratpoison/).
Note that pWM is still in beta.

Each terminal acts like an independent window with its own output, input, scroll state, and command history. The layout is automatically reorganized as follows:

- One terminal: the whole screen is used
- Two terminals: vertical split
- Three terminals: one on the left and two on the right
- Four terminals: 2x2 grid

## Automatic startup

After login, `kmain.c` starts pWM. A terminal is created automatically when the manager starts.

If pWM is stopped, the system returns to the minimal kernel shell. From that shell, it can be restarted with `pwm --start`.

## The `pwm` command

Syntax:

```text
pwm [--help|--info|--reload|--start|--stop]
```

The alias `pmw` is also accepted.

### `pwm --help`

Shows the command syntax and a list of available options.

If no arguments are provided, `pwm` displays the same help text.

### `pwm --info`

Shows the pWM description defined in `etc/pwm-info`, followed by the current manager status.

The file `etc/pwm-info` is embedded into the kernel image during the build, so the information displayed by `--info` stays centralized in a single place.

### `pwm --reload`

Resets the active terminal view and requests a redraw of the pWM desktop.

It does not restart the kernel and does not close any open terminal.

### `pwm --start`

Starts pWM from the minimal shell.

If pWM is already active, the command reports that a second instance cannot be started.

### `pwm --stop`

Stops pWM and returns control to the minimal kernel shell.

The terminals and their in-memory content are recreated when pWM is restarted. This command does not shutdown or reboot the machine.

## pWM controls

| Key | Action |
| --- | --- |
| `Ctrl+Q` | Opens a new terminal up to a maximum of four windows |
| `Ctrl+C` | Closes the active terminal |
| `Ctrl+Left` | Focuses the previous terminal |
| `Ctrl+Right` | Focuses the next terminal |
| Up arrow | Shows the previous command in history |
| Down arrow | Shows the next command in history |
| Mouse wheel | Scrolls the active terminal |

Each terminal keeps a command history of up to 16 entries. The mouse wheel is the only mouse interaction currently used by pWM; cursor movement and clicks do not change the layout.

## Commands inside terminals

pWM terminals use the same handlers as the kernel shell, so the usual commands are available, including:

```text
help       Show the available commands
ls         List files and directories
cd         Change directory
cat        Read a file
clear      Clear the active terminal
touch      Create a file
mkdir      Create a directory
nano       Edit a file
echo       Print text
fs         Show filesystem status
shutdown   Power off the system
reboot     Restart the system
ecc...
```
