## Description [![Build Status](https://travis-ci.org/koekeishiya/kwm.svg?branch=master)](https://travis-ci.org/koekeishiya/kwm)

**NOTE:** The master branch is considered stable and can be used instead of the latest release version by people who wish to do so.

**NOTE:** Main development focus has been shifted to a [new window manager](https://github.com/koekeishiya/chunkwm) based on plugin architecture.

[**Kwm**](https://koekeishiya.github.io/kwm) started as a simple project to get true focus-follows-mouse support on OSX through event tapping.
It is now a tiling window manager that represents windows as the leaves of a binary tree.
*Kwm* supports binary space partitioned, monocle and floating spaces.

*Kwm* uses the event taps API (Quartz Event Services) to observe, filter and alter user input events prior
to their delivery to a foreground application.

*Kwm* runs a local daemon to read messages and trigger functions.
*Kwmc* is used to write to *Kwm*'s socket, and must be used when interacting with and configuring how *Kwm* works.
[Khd](https://github.com/koekeishiya/khd) can be used to create keybindings to perform some *Kwmc* command.
More info can be found [in the example dir](https://github.com/koekeishiya/kwm/tree/master/examples).

For in depth information, [**click here**](https://koekeishiya.github.io/kwm).
For sample configurations and other useful scripts, check out the [wiki](https://github.com/koekeishiya/kwm/wiki).
You can also drop by the channel **##kwm** on [freenode](http://webchat.freenode.net).

*Kwm* requires access to the OSX accessibility API.
Tested on El Capitan (10.11.1 - 10.11.6).

![img](https://cloud.githubusercontent.com/assets/6175959/22448246/62db5a08-e758-11e6-9a87-cf9fc5c6f51b.png)
For more screenshots, [click here.](https://github.com/koekeishiya/kwm/issues/2)

The bar seen in the above screenshot can be found [here](https://github.com/koekeishiya/nerdbar.widget).

## Install

**NOTE:** Kwm requires ['Displays have separate spaces'](https://support.apple.com/library/content/dam/edam/applecare/images/en_US/osx/separate_spaces.png) to be enabled.

A codesigned binary release is available through Homebrew

    brew install koekeishiya/formulae/kwm

Add the example [`kwmrc`](https://github.com/koekeishiya/kwm/blob/master/examples/kwmrc) file to `~/.kwm/kwmrc` to get started.

Manage Kwm using brew services

    brew services start kwm

## Configuration

**NOTE**: [Documentation](https://koekeishiya.github.io/kwm/kwmc.html) of available commands

The default configuration file is `$HOME/.kwm/kwmrc` and is a script that contains *kwmc* commands
to be executed when *Kwm* starts.

A different path can be used by running `kwm -c /path/to/kwmrc` or `kwm --config /path/to/kwmrc`,
in which case it would probably be a good idea to set the [directories *Kwm* uses](https://github.com/koekeishiya/kwm/issues/191) for various settings.

A sample config file can be found within the [examples](examples) directory.
Any error that occur during parsing of the config file will be written to **stderr**.

## Development

**NOTE:** Kwm requires ['Displays have separate spaces'](https://support.apple.com/library/content/dam/edam/applecare/images/en_US/osx/separate_spaces.png) to be enabled.

**NOTE:** Requires Xcode-8 command line tools

Build *AXLib* only

      make install-lib  # release version, runs cleanlib
      make lib          # debug version

Build *Kwm* (also builds *AXLib* if required)

      make install      # release version, runs cleankwm
      make              # debug version

Remove temporary build artifacts

      make clean        # runs cleanlib and cleankwm
      make cleanlib     # remove axlib artifacts
      make cleankwm     # remove kwm artifacts

Start *Kwm* on login through launchd

      edit /path/to/kwm on line 9 of examples/com.koekeishiya.kwm.plist
      cp examples/com.koekeishiya.kwm.plist ~/Library/LaunchAgents/

Manually manage *Kwm* using launchctl

      launchctl load -w ~/Library/LaunchAgents/com.koekeishiya.kwm.plist

### Donate
*Kwm* will always be free and open source, however, some users have expressed interest in some way to show their support.
If you wish to do so, I have set up a patreon [here](https://www.patreon.com/aasvi).
