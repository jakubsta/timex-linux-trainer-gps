#!/usr/bin/env python

from endomondo import MobileApi, Workout, TrackPoint
import lib.tcx as tcx
import argparse
import keyring
import datetime
import logging, sys, os, getpass, shelve
from functools import partial

import timex

def endo_conv_point(point):
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

def upload_file(endomondoapi, file):
    points = reduce(lambda memo, lap: memo + lap['samples'], file['laps'], [])
    map(endo_conv_point, points)

    workout = Workout()
    workout.sport = 0
    workout.points = points
    workout.start_time = datetime.datetime.utcfromtimestamp(file['start'])
    workout.distance = points[-1]['dist'] 
    workout.ascent = file['ascent']
    workout.descent = file['descent']
    workout.duration = file['duration']

    endomondoapi.post_workout(workout=workout, properties={'audioMessage': 'false'})
    if workout.id:
        print "Saved! %d"%workout.id

def configure_endomondo(conf):
    endomondoapi = MobileApi()
    email = ''
    password = ''

    while not email:
	email = raw_input('Please enter your endomondo email: ')

    while not password:
	password = getpass.getpass()
	if not password:
	    print 'Password can\'t be empty'

    auth_token = ''
    try:
	auth_token = endomondoapi.request_auth_token(email=email, password=password)
    except:
	pass
    if not auth_token:
	print 'Did not receive valid authentication token. Try one more time.'
    else:
	conf['endomondo'] = True
	conf['email'] = email
	keyring.set_password("endomondo", email, auth_token)
	print 'Configured!'

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configure', action='store_true')
    parser.set_defaults(configure=False)
    args = parser.parse_args()

    conf = shelve.open('./.conf')
    if not conf.has_key('endomondo'):
	conf['endomondo'] = False

    if args.configure: 
        print 'Configure'
	configure_endomondo(conf);
	conf.close()
        sys.exit()

    files = timex.read('/dev/timex')

    activities = map(file_to_activity, files)
    map(create_tcx_file, activities)

    if not conf['endomondo']:
	conf.close()
	sys.exit()

    # Endomondo upload
    endomondoapi = MobileApi()
    auth_token = keyring.get_password('endomondo', conf['email'])
    endomondoapi.set_auth_token(auth_token)

    workouts = map(partial(upload_file, endomondoapi), files)

    conf.close()
