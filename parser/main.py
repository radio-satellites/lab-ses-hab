import socket
import time
import sys
from datetime import datetime
import parser

_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_s.settimeout(1)
_s.connect(("127.0.0.1",7322))

outputfile = open("log.txt",'w')

message = ""

while True:
    try:
         _char = _s.recv(1).decode()
         #print(_char)
         message += _char
         #print(message)       
         if _char == "\n" or _char == "1":
            try:
                print(parser.parse(message))
            except:
                print("CANNOT PARSE MESSAGE :(")
            message = ""
                
             
             
    except:
        print("NO CHARS RECEIVED...")
        pass
