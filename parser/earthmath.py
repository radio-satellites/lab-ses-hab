#main.py - calculate the distance, elevation, and bearing of an object at point a, received from point b
#based on https://github.com/projecthorus/sondehub-tracker/blob/master/js/tracker.js. THANK YOU SO MUCH!!!!

import math

DEG_TO_RAD = math.pi / 180.0
EARTH_RADIUS = 6371000.0


#calculates look angles between two points
# format of a and b should be {lon: 0, lat: 0, alt: 0}
#returns {elevention: 0, azimut: 0, bearing: "", range: 0}
#
#Copyright 2012 (C) Daniel Richman; GNU GPL 3
def calculate_lookangles(a, b):
  #degrees to radii
  a['lat'] = a['lat'] * DEG_TO_RAD
  a['lon'] = a['lon'] * DEG_TO_RAD
  b['lat'] = b['lat'] * DEG_TO_RAD
  b['lon'] = b['lon'] * DEG_TO_RAD

  d_lon = b['lon'] - a['lon']
  sa = math.cos(b['lat']) * math.sin(d_lon)
  sb = (math.cos(a['lat']) * math.sin(b['lat'])) - (
    math.sin(a['lat']) * math.cos(b['lat']) * math.cos(d_lon))
  bearing = math.atan2(sa, sb)
  aa = math.sqrt(math.pow(sa, 2) + math.pow(sb, 2))
  ab = (math.sin(a['lat']) * math.sin(b['lat'])) + (
    math.cos(a['lat']) * math.cos(b['lat']) * math.cos(d_lon))
  angle_at_centre = math.atan2(aa, ab)
  great_circle_distance = angle_at_centre * EARTH_RADIUS

  ta = EARTH_RADIUS + a['alt']
  tb = EARTH_RADIUS + b['alt']
  ea = (math.cos(angle_at_centre) * tb) - ta
  eb = math.sin(angle_at_centre) * tb
  elevation = math.atan2(ea, eb) / DEG_TO_RAD

  #Use Math.coMath.sine rule to find unknown side.
  distance = math.sqrt(
    math.pow(ta, 2) + math.pow(tb, 2) -
    2 * tb * ta * math.cos(angle_at_centre))
  bearing /= DEG_TO_RAD
  
  if bearing < 0:
    bearing = 360-bearing #I THINK? I'm not sure
  if bearing > 360:
     bearing = bearing - 360
  

  return {
    'elevation': elevation,
    'azimuth': bearing,
    'range': distance,
    'great_circle_distance': great_circle_distance
  }


