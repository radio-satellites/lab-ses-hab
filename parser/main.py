import socket
import time
import sys
from datetime import datetime
import parser
import collections
import time

print("LAB SES 1 MISSION TELEMETRY DECODER/DASHBOARD")
time.sleep(0.5)
print("By VE3SVF")
print("\n")

_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_s.settimeout(1)
num_nochars = 0
try:
    _s.connect(("127.0.0.1",7322))
except:
    print("ERROR: COULD NOT CONNECT TO FLDIGI")

packet = ""

while True:
    try:
         _char = _s.recv(1).decode()
         num_nochars = 0
         #print(_char)
         packet = packet + _char #Update buffer
         #print(message)
         if _char == "\n":
             try:
                pressure,altitude,temperature,latitude,longitude,frame_num = parser.parse_string(packet)
                print("=======================NEW FRAME=======================")
                print("----HEADER----")
                print("Frame number: "+str(frame_num))
                print("--------------")
                print("--------------PAYLOAD--------------")
                print("Pressure: "+str(pressure)+" hPa")
                print("Altitude: "+str(altitude)+" m above sea level")
                print("Temperature (inside): "+str(temperature)+" *C")
                print("-----------------------------------")
                print("------Location Data------")
                print("Latitude: "+str(latitude))
                print("Longitude: "+str(longitude))
                print("-------------------------")
                print("=======================================================")
                print("\n")
                packet = ""
             except Exception as e:
                print(e)
                print("ERROR: CANNOT PARSE MESSAGE :(")
                packet = "" #Maybe

                
             
             
    except:
        if num_nochars >= 5:
            print("WARN: NO CHARS RECEIVED...")
            num_nochars = num_nochars+1
        pass
