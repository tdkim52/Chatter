# chatter
Terminal Based Chat Link Client

Run on 132 column by 43 row terminal window.
Program utilizes unbuffered stream and no echo of typed characters in insert mode

TODO:

		Fix bugs when running with flags causing unpredicted behavior
		Fix bugs with some commands not working correctly (i.e. external filters, read)
		Add :source command to run scripts
		Fix: non-blocking when in insert mode and after ':' is typed
		Comment code for proper documentation
		Remove debugging statements/blocks of code

Command line flags:

    -noleft       : no incoming connection
    -noright      : no outgoing connection
    -raddr value  : specify outgoing (right) address/DNS
    -laddr value  : specify valid incoming (left) address/DNS , '*'
    -lrport value : specify left remote source port, '*'
    -llport value : specify left local port
    -loopr        : inject outgoing right data into incoming data stream from right
    -loopl        : inject outgoing left data into data stream from left
    -dsplr        : display data going left to right
    -dsprl        : display data going right to left
  
Interactive insert mode commands:

    i   : enter insert mode
    esc : exit insert mode and return to command mode (escape key)
    
    :outputl  : outputs data to left connection client
    :outputr  : outputs data to right
    :output   : display current direction of data flow
    :lpair    : display currently connected tcp pair for left side
    :rpair    : display currently connected tcp pair for right side
    :loopr    : -loopr
    :loopl    : -loopl
    :dropr    : drop the right side connection
    :dropl    : drop the left side connection
  
    :connectr IP [port] : create connection with 'IP' on their 'port'(optional) on right side
    :connectl IP [port] : '' left side
    :listenl [port]     : listen for connection on left local 'port', Default port 36753
    :listenr [port]     : '' right side
  
    :read filename : read conents of file and write in current output direction
  
    :llport port : bind to local 'port' for left side passive connection
    :rlport port : bind to local 'port for right side
    :lrport port : accept a connection on left if remote computer has source 'port'
    :rrport port : used 'port' to connect to remote machine for right side
  
    :laddr IP : accept connection on left if remote matches 'IP'
    :raddr IP : use 'IP' as address to connect
  
    :stlrnp     : strip non-printable characters moving left to right
    :strlnp     : '' right to left
    :stlrnpxeol : '' left to right except CR/LF characters
    :strlnpxeol : '' right to left
  
    :loglrpre filename  : log left to right traffic before any filtering to 'filename'
    :logrlpre filename  : right to left
    :loglrpost filename : log left to right traffic after any filtering to 'filename'
  
    :externallr command : run 'command' as external filter for left to right traffic
    :externalrl command :  '' right to left traffic
    ('command' cannot be multiple commands connected with pipes or redirection or shell meta-characters)
    ([grep -i abc] would be valid)
  
    :q terminates the program
  
