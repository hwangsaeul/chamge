[![Build Status](https://dev.azure.com/hwangsaeul/hwangsaeul/_apis/build/status/hwangsaeul.chamge?branchName=master)](https://dev.azure.com/hwangsaeul/hwangsaeul/_build/latest?definitionId=5&branchName=master)

Chamge
------

Chamge[tʃɑmge] is a connection broker API

## Getting Started

### Install meson and ninja
Meson 0.40 and newer is required.

### Build Chamge
You can get library and executables built running:
```
meson build
ninja -C build
```

### Broker types
There are three modules for connection broker API. Theses module's states are enrolled, activated, deactivated, delisted.

#### Edge
Edge is a connection broker module for srt source devices. 

#### Arbiter
Arbiter is a connection broker module for srt management servers.

#### Hub
Hub is a connection broker module for srt relay servers.

### Examples
There are dummy files in examples folder. These are helper examples to implement edge/hub/arbiter.

#### Test Sequence of examples
1. Run arbiter-dummy : Arbiter's status will go to activated status automatically. It will wait for the messages from edge/hub to enroll/activate.
2. Run edge-dummy/hub-dummy : It will send enroll/activate to arbiter-dummy, and will be activated status after get response from arbiter-dummy.
3. Input user-command : arbiter-dummy will send the sample user command according to user's key input. If you want to check what kind command you can use, try "h".


### Agents
Roll of agents are for communicating with another process. Agents include D-BUS api that is created by code generater from defined xml in the path of chamge/dbus/. Manager server's RESET Api communicate with Arbiter via D-BUS(chamge-arbiter-agent).
