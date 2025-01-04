
# Pico-W Runtime Wifi Configuration

To connect the Pico-W to the WLAN, you need to specify the network name and password when building a Pico image.

According to the documentation:<br>
*If you are building applications with the C/C++ SDK for Raspberry Pi Pico W and, to connect to a network you will need to pass -DPICO_BOARD=pico_w -DWIFI_SSID="Your Network" -DWIFI_PASSWORD="Your Password" to CMake.*

The same applies if the Pico is to be given a fixed IP address.
*... you can specify the IP address when building a pico image by defining CYW43_DEFAULT_IP_STA_ADDRESS.*

This can be done for testing purposes and for individual devices.
However, if multiple devices are to be used on different Wi-Fi networks, an unique image must be created for each device.


**This software makes it unnecessary to know the network name, password and - if required - IP address, network mask and default gateway when compiling. These can be set directly on the Pico-W and also modified afterwards.**

To do this, the Pico-W is first started in "Access Point Mode". In this mode, it provides a web server with a page for entering the data. The user connects to the WLAN of the Pico-W (picow-config, no password) and opens the configuration page in a browser at 192.168.0.1.

After completing the settings (press the 'Setup' button), the data is stored in flash, 'Access Point Mode' is terminated and the PICO-W connects to the specified WLAN in 'Station Mode' using the data provided.

The next time the PICO-W boots, it will check if the flash contains a valid configuration, and if so, it will skip the 'Access Point Mode' step and start immediately in 'Station Mode'.

To change the stored data afterwards, the 'SETUP_GPIO' input (GPIO22) must be pulled to GND for at least 3 seconds during the reboot. This will execute the "Access Point Mode" step. Now the data can be changed as described above.

# DHCP versus fixed IP:
If you require the user to enter a fixed IP address (which means you don't need DHCP support), set LWIP_DHCP to 0 in lwiopts.h. This will reduce the size of the code.

# How to use this software:
Copy the `wifi_setup` subdirectory into your project.
Copy the lines in `main.c` between the "configuration code starts here / ends here" tags to somewhere near the beginning of your code.
Use the code between the "Typical connection sequence starts/ends here" tags as a template.

Do not forget to include `access_point.h`.
"CmakeLists.txt" shows the necessary directives, directories to include in the search path and link libraries.

# The demo software:
`main.c` and `tcp-server.c` together form a simple example. After completing the configuration described above, a TCP echo server is started on port 4711.

How to build:
See chapter 8 of the 'Getting Started with Raspberry Pi Pico' for instructions.

1. Create a directory for your project, alongside the pico-sdk directory.
2. Download 'Wi-Fi Configure' into this directory.
3. Then copy the pico_sdk_import.cmake file from the external folder in your pico-sdk installation to your project folder.
4. Now follow the usual steps:
$ mkdir build
$ cd build
$ cmake ...
$ make

You can use the `client` program in the "linux" subdirectory to send data to the server.
To create `client`, it is usually sufficient to change to the `linux` subdirectory and run `make client`.

Run it with : `./client IP-ADDRESS 4711`, enter any text and send it with ".<RTN\>".<br>
To exit, type "q<RTN\>".<br>
To erase the configuration from the flash and return the pico to "unconfigured", use the special command "erase!”
In addition, the server will respond to the "conf!" command with its IP address.

Note: If you have not configured a fixed IP address, you need to find out the address either by viewing the debug output on a terminal, using the `nmap` utility, or from your wireless router.<br>

# Modify The Web Pages:
For the Pico-W, the HTML files must be converted to binary form. The Perl script "wifi_setup /external/makefsdata" is used for this. Do not use it directly, but change to the subdirectory "wifi_setup" and run the shell script "rebuild_fs.sh".
This will create the file "my_fsdata.c" which will be included in "pico-sdk/lib/lwip/src/apps/http/fs.c" during compilation.

**ATTENTION:**
"makefsdata" translates ALL FILES in the access_point/fs directory, not just html pages. If you get strange error messages when compiling, check if there are other files in that directory. Check that there are no hidden `.*` files.

The HTML page names (without the extensions) must result in a valid variable name. Do not use "my-page.html". Use "my_page.html".

# Copyright:
Gerhard Schiller (c) 2025 gerhard.schiller@protonmail.com

# Licences:
This software is licensed under the "The 3-Clause BSD Licence".<br>
See LICENSE.TXT for details.

This license also applies to the pico-sdk examples used as a basis to create this software, with the exception of the dhcp-server, which is licensed under the 'MIT License'.

