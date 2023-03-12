

def parse_string(string):
    parameters = string.split(",")
    num_correct = 0
    try:
        pressure = float(parameters[1]) #Recover the data back
        num_correct = num_correct+1
    except:
        pressure = 0
    try:
        altitude = float(parameters[2])
        num_correct = num_correct+1
    except:
        altitude = 0
    try:
        temperature = float(parameters[3])
        num_correct = num_correct+1
    except:
        temperature = 0
    try:
        latitude = float(parameters[4])/1000000
        if latitude > 20 and latitude < 100:
            num_correct = num_correct+1
            
    except:
        latitude = 0
    try:
        longitude = float(parameters[5])/-1000000
        if longitude < -10 and longitude > -100:
            num_correct = num_correct+1
    except:
        longitude = 0
    if num_correct > 0:
        return pressure,altitude,temperature,latitude,longitude
    else:
        return 0
        
    

