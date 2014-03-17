expircd
=======

ExpIRCd is INCOMPLETE; An experimental IRCd written in C, aiming to be minimalist and portable (no redundant features such as dynamic configuration, reverse resolution, OS-specific socket handling).


Example configuration is in default_config.h.

To compile using gcc as your compiler, on a Windows machine with default_config.h as your config:

gcc -c -DCONFIG='"default_config.h"' --std=c99 node.c
gcc -DCONFIG='"default_config.h"' --std=c99 main.c node.o -lws2_32

On other OSes, comment out the WIN32 and WINVER preprocessor definitions from default_config.h prior to compilation.