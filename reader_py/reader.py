#!/usr/bin/env python

from endomondo import MobileApi, Workout, sports, TrackPoint
import lib.tcx as tcx
import keyring
import datetime
import logging, sys, os

import timex

def convPoint(point):
    point['time'] = datetime.datetime.utcfromtimestamp(point['time'])
    point['dist'] /= 1000
    point['speed'] *= 3.6
    return point

# create a new directory to store the exported files in
def create_directory(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

# create the TCX file for the specified workout
def create_tcx_file(activity):
    directory_name = 'exports'
    name = '%s.tcx'%str(activity.start_time) 
    create_directory(directory_name)
    filename = os.path.join(directory_name, name)
    writer = tcx.Writer()
    tcxfile = writer.write(activity)
    if tcxfile:
        with open(filename, 'w') as f: 
            f.write(tcxfile)

def points_formater(samples):
    points = []
    for point in samples:
        w = tcx.Trackpoint()
        w.timestamp = datetime.datetime.utcfromtimestamp(point['time'])
        w.latitude = round(point['lat'], 6)
        w.longitude = round(point['lng'], 6)
        w.altitude_meters = round(point['alt'], 2)
        w.distance_meters = round(point['dist'], 2)
        w.heart_rate = point['hr']
        w.speed = round(point['speed'], 3)
        points.append(w)
    return points

def lap_formater(lap):
    points = points_formater(lap['samples'])

    l = tcx.ActivityLap()
    l.start_time = points[0].timestamp
    l.timestamp = points[0].timestamp
    l.total_time_seconds = round(lap['duration'], 2) 
    l.min_altitude = lap['altMin']
    l.max_altitude = lap['altMax']
    l.distance_meters = lap['dist'] 

    # l.max_heart = to_float(data[13])
    # l.avg_heart = to_float(data[14])
    
    l.trackpoints = points
    return l

def file_to_activity(file):
    activity = tcx.Activity()
    activity.sport = 0

    startTime = datetime.datetime.utcfromtimestamp(file['start'])
    activity.start_time = startTime 
    
    for lap in file['laps']:
        activity.laps.append(lap_formater(lap))
    
    return activity

if __name__ == '__main__':
    files = timex.read('/dev/timex')
    
    activities = map(file_to_activity, files)
    map(create_tcx_file, activities)

    # logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
    # endomondoapi = MobileApi()
    # auth_token = keyring.get_password('endomondo', 'kubafr2@gmail.com')
    # endomondoapi.set_auth_token(auth_token)

    # points = files[-1]['samples']
    # map(convPoint, points)
    # workout = Workout()
    # workout.sport = 0
    # workout.name = 'test'
    # workout.points = points
    # workout.start_time = datetime.datetime.utcfromtimestamp(files[-1]['start'])
    # workout.distance = points[-1]['dist'] 
    # workout.ascent = files[-1]['ascent']
    # workout.descent = files[-1]['descent']
    # workout.duration = files[-1]['duration']

    # endomondoapi.post_workout(workout=workout, properties={'audioMessage': 'false'})
    # if workout.id:
    #     print "Saved! %d"%workout.id
