The program will successfully act as a transparent proxy, parse request and response headers
and can block words, in other words, act as a censorship proxy. Dynamic blocking working but
limited to one word. Pictures and all text formats will successfully be loaded. The program is
able to handle HTTP redirections regarding keywords that are blocked.

To compile the program, you will have to run gcc -o proxy proxy.c
Now after you have compiled the program, you will have to run it by typing in ./proxy <port> with
the port being the value you wish to run the proxy on.
The proxy will require a telnet console to give the unblock, block and exit commands and the
format for the command block will be BLOCK <keyword> and unblock will be UNBLOCK
<keyword> with the keyword being the word you wish to either block or block.
You are required to configure your browser to be running the localhost IP address (127.0.0.1 or
localhost) and the port that you specified earlier.
  
Achieved a grade of 37/40
Note: repo reuploaded to hide sensitive information therefore has no commit history
