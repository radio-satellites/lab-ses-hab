#example_string = "$$,99900,12100,2300,43669380,79521368" #test string

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
        else:
            latitude = 0
            
    except:
        latitude = 0
    try:
        longitude = float(parameters[5])/-1000000
        if longitude < -10 and longitude > -100:
            num_correct = num_correct+1
        else:
            longitude = 0
    except:
        longitude = 0
    try:
        frame_num = int(parameters[6])
        if frame_num < 0:
            frame_num = 0
    except:
        frame_num = 0
        
    if num_correct > 0:
        return pressure,altitude,temperature,latitude,longitude,frame_num
    else:
        return 0
        
    

