#example_string = "$$,99900,12100,2300,43669380,79521368" #test string

def parse_string(string):
    finished = False
    parameters = string.split(",")
    try:
        pressure = float(parameters[1])/100 #Recover the data back
    except:
        pressure = 0
    try:
        altitude = float(parameters[2])/100
    except:
        altitude = 0
    try:
        temperature = float(parameters[3])/100
    except:
        temperature = 0
    try:
        latitude = float(parameters[4])/1000000
    except:
        latitude = 0
    try:
        longitude = float(parameters[5])/-1000000
    except:
        longitude = 0
    
    if "." in parameters[6]:
        #End of telemetry detected!
        finished = True
    return pressure,altitude,temperature,latitude,longitude,finished

