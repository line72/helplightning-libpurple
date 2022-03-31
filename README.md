# helplightning-libpurple

A Help Lightning protocol pluging for libpurple IM clients

## Installation/Configuration

### Linux/macOS

1. Install libpurple (pidgin, finch, etc.), including necessary development components on binary distros (`libpurple-devel`, `libpurple-dev`, etc.);
1. Clone this repository with `git clone https://github.com/line72/helplightning-libpurple.git`, run `cd helplightning-libpurple`, then run `sudo make install` or `make install-user`.

### Workspace Selection

The plugin only supports a single workspace at a time. If your account is in multiple workspaces, the plugin uses the first one available. You can override this in the account options.

## Limitations

Currently, only one on one chats are supported. Multi-user/Group Help Lightning chats do not currently show up.

## License

Copyright (c) 2022 Marcus Dillavou <line72@line72.net>

Licensed under the GPLv3 or later.
