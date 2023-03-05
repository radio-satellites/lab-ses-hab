import socket
import time
import sys
from datetime import datetime
import parser
import collections

_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_s.settimeout(1)
_s.connect(("127.0.0.1",7322))

outputfile = open("log.txt",'w')

d = collections.deque(maxlen=42)

def update_cyclic_buffer(string):
    d.append(string)
    return 0

def return_cyclic_buffer():
    return list(d)

def return_cyclic_buffer_string():
    return ''.join(return_cyclic_buffer())
def flush_buffer():
    d = collections.deque(maxlen=42)

while True:
    try:
         _char = _s.recv(1).decode()
         #print(_char)
         update_cyclic_buffer(_char) #Update cyclic parsing buffer
         #print(message)       
         try:
            print(return_cyclic_buffer_string())
            print(parser.parse_string(return_cyclic_buffer_string()))
         except Exception as e:
            print(e)
            print("CANNOT PARSE MESSAGE :(")

                
             
             
    except:
        print("NO CHARS RECEIVED...")
        pass
