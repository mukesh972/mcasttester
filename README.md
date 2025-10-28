```markdown

# mcasttester -Miracast test application with event subscription (C++ / CMake)

This application sends real JSON-RPC commands to a Thunder Controller and subscribes to events over WebSocket. You can activate/deactivate plugins, call Miracast methods (setEnable, playRequest, stopRequest, updatePlayerState, etc.), and the application will print incoming events in real time. When a MiracastService client connection request is received, its mac/name are recorded so you can accept/reject from the terminal.

Requirements
- boost
- websocketpp
- jsoncpp
- cmake (for building)

Build
```bash
Yocto Build
If you are building using Yocto, use this command to checkout:

devtool add --autorev mcasttester https://github.com/mukesh972/mcasttester.git --srcbranch main

Add the following line in the recipe:

inherit cmake pkgconfig
DEPENDS += "jsoncpp websocketpp systemd boost curl"
RDEPENDS_${PN} += "jsoncpp"

```

Run
```bash
# optional: pass controller URL as argv1, otherwise uses compile-time THUNDER_JSONRPC_URL
./mcasttester http://127.0.0.1:9998/jsonrpc
```
Usage
Verification in middleware layer:
Based on ENABLE_MIRACAST distro, wlan-p2p service is added and MiracastService and MiracastPlayer plugins are added.

Steps:
1. Create the auto connection flag

touch /opt/miracast_autoconnect

2. Enable Miracast ports

Add below lines at line 132 /lib/rdk/iptables_init:

    # MiracastService plugin need to communicate with client through below ports
    # 7236 - RTSP session communication
    # 1990 - UDP streaming for Mirroring
    # 67 - DHCP server to provide ip to clients through P2P group interface
    $IPV4_BIN -A INPUT -p tcp -s 192.168.0.0/16 --dport 7236 -j ACCEPT
    $IPV4_BIN -A INPUT -p udp -s 192.168.0.0/16 --dport 1990 -j ACCEPT
    $IPV4_BIN -A INPUT -i p2p+ -p udp --dport 67 -j ACCEPT

Sync and reboot. 


3. Set Environmental variables for Westeros for VideoOutput and Run Westeros server
Note: below is Amlogic environment variables, please use the platform specific export commands

export LD_PRELOAD=libwesteros_gl.so.0.0.0
export WESTEROS_GL_GRAPHICS_MAX_SIZE=1920x1080
export WESTEROS_GL_MODE=3840x2160x60
export WESTEROS_GL_USE_REFRESH_LOCK=1
export WESTEROS_GL_USE_AMLOGIC_AVSYNC=1
export WAYLAND_DISPLAY=main0
export WESTEROS_SINK_AMLOGIC_USE_DMABUF=1
export WESTEROS_SINK_USE_FREERUN=1
export WESTEROS_SINK_USE_ESSRMGR=1
export AAMP_ENABLE_WESTEROS_SINK=1
export AAMP_ENABLE_OPT_OVERRIDE=TRUE
export FORCE_SVP=TRUE
export FORCE_SAP=TRUE
export XDG_RUNTIME_DIR=/tmp
export AAMP_ENABLE_WESTEROS_SINK=1
export GST_ENABLE_SVP=1
westeros --renderer libwesteros_render_embedded.so.0.0.0 --display main0 --embedded --window-size 1920x1080 --noFBO &


4. Copy mcasttester application in /opt/ directory and give executable permission.
chmod 777 mcasttester

5.From /opt/ directory run mcasttester


root@mesonsc2-5:/opt# ./mcasttester
Miracast CLI w/ events
Controller HTTP JSON-RPC URL: http://127.0.0.1:9998/jsonrpc
WebSocket event URL (derived): ws://127.0.0.1:9998/jsonrpc
Available commands:
  help
  1. activate_service
  2. activate_player
  3. set_enable
  4. accept      # accept last seen client (from events)
  5. quit
Event listener started.
> 1
[activate_service] {"id":1,"jsonrpc":"2.0","result":null}
> 2
[activate_player] {"id":2,"jsonrpc":"2.0","result":null}
> 3
[set_enable] {"id":3,"jsonrpc":"2.0","result":{"message":"Successfully enabled the WFD Discovery","success":true}}
> 4
No client known from events. Wait for onClientConnectionRequest.
>
>
> 5
Shutting down event listener...
Exit
root@mesonsc2-5:/opt#

NB: After giving step "3. set_enable",  connect mobile device casting and check for the device list, select and wait for "connecting" in mobile, then give step "4. accept". Now navigate and play videos in mobile.

```
