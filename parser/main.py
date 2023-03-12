import socket
import time
import sys
from datetime import datetime
import parser
import collections

_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_s.settimeout(1)
try:
_s.connect(("127.0.0.1",7322))
except:
    print("ERROR: COULD NOT CONNECT TO FLDIGI")

packet = ""

while True:
    try:
         _char = _s.recv(1).decode()
         #print(_char)
         packet = packet + _char #Update buffer
         #print(message)
         if _char == "\n":
             try:
                print(parser.parse_string(packet))
             except Exception as e:
                print(e)
                print("ERROR: CANNOT PARSE MESSAGE :(")
                packet = "" #Maybe

                
             
             
    except:
        print("WARN: NO CHARS RECEIVED...")
        pass
