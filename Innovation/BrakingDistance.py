# -*- coding: utf-8 -*-
"""
BrakingDistance.py

Functions which are used to determine braking distances and depth

Created on Tue Mar 31 21:58:34 2026

@author: KTam1
"""
import numpy as np
import cv2


"""
getImageHeight

Function which estimates image height using actual distance information

Inputs:
- D: Braking distance required
- h: Height of the camera above track level (metres)
- img_height: Height of the image (pixels)

Output: Image y-distance (pixels) above the bottom of the image
"""
def getImageHeight(D, h, img_height):   
    alpha = np.deg2rad(26.15)
    delta = np.deg2rad(10.0)
    beta = np.deg2rad(90.0)-alpha-delta

    return -(img_height-1)/(2*alpha)*(np.arctan(D/h)-beta)+img_height-1

"""
getWarningHeights

Function which determines which y-values would be 

Inputs:
- velocity: Velocity which the loco is travelling at (m/s)
- a_decel: Acceleration (negative for deceleration) during braking (i.e., use negative value)
- t_warning: Time before impact (warning signal) (seconds)
- t_danger: Time before impact (danger signal) (seconds)
- safety_factor: Multiplied to the distances for safety (coefficient)
- L: Actual direct distance (i.e., hypotenuse) (metres)
- m: gradient of the relationship between y and L (should be proportional)
- c: y-intercept of the relationship between y and L (should be proportional)
- h: Height of the camera above track level (metres)
- img_height: Height of the image (pixels)

Output:
    - NOTE: The y-axis here has been flipped to be from the top, because
    this is how the images are indexed
- y_warning: Height below the top of image when warning should be given (pixels)
- y_danger: Height below the top of image when danger should be given (pixels)
- d_warning: Actual distances away calculated for warning (metres)
- d_danger: Actual distances away calculated for danger (metres)
"""
def getWarningHeights(velocity, a_decel, t_warning, t_danger, safety_factor, h, img_height):
    # Derived from SUVAT equations, assuming constant deceleration
    d_brake = -velocity**2/(2*a_decel) # Braking distance
    
    # Assumes the vehicle travels t-seconds at velocity before applying full brakes
    d_warning = (velocity*t_warning + d_brake)*safety_factor # Warning distance
    d_danger = (velocity*t_danger + d_brake)*safety_factor # Danger distance

    # Get the y-distances
    y_warning = getImageHeight(d_warning, h, img_height) # Image height inverse
    y_danger =  getImageHeight(d_danger, h, img_height) # Image height inverse

    return (y_warning, y_danger, d_warning, d_danger)


"""
highlightTrack

Function which annotates the tracks, and highlights whether it is safe or not

Inputs:
- img: Original image
- track_points: Organised track coordinates
- y_warning: Height below the top of image when warning should be given (pixels)
- y_danger: Height below the top of image when danger should be given (pixels)
- d_warning: Actual distances away calculated for warning (metres)
- d_danger: Actual distances away calculated for danger (metres)

Output: Highlighted image
"""
def highlightTrack(img, track_points, y_warning, y_danger, d_warning, d_danger):
    # Classify the track region
    danger_track = track_points[track_points[:, 1] >= y_danger]
    warning_track = track_points[(track_points[:, 1] >= y_warning) & (track_points[:, 1] < y_danger)]
    okay_track = track_points[track_points[:, 1] < y_warning]
    
    # Colour the tracks with different colours
    if len(danger_track) > 0:
        cv2.fillPoly(img, np.int_([danger_track]), (255,0,0))

    if len(warning_track) > 0:
        cv2.fillPoly(img, np.int_([warning_track]), (255,151,0))

    if len(okay_track) > 0:
        cv2.fillPoly(img, np.int_([okay_track]), (0,200,255))
        
    return img


"""
drawTrack

Function which draws the track with braking distance highlighted

Inputs:
- img: Original image
- ploty: List of reference y-values
- left_fit: x-values of the left rail (from the track detection)
- right_fit: x-values of the right rail (from the track detectoin)
- y_warning: Height below the top of image when warning should be given (pixels)
- y_danger: Height below the top of image when danger should be given (pixels)
- d_warning: Actual distances away calculated for warning (metres)
- d_danger: Actual distances away calculated for danger (metres)

Outputs:
- color_img: Image with track highlighted
- track_points: Organised track coordinates
"""
def drawTrack(img, ploty, left_fit, right_fit, y_warning, y_danger, d_warning, d_danger):
    color_img = np.zeros_like(img)
    
    left = np.array([np.transpose(np.vstack([left_fit, ploty]))])
    right = np.array([np.flipud(np.transpose(np.vstack([right_fit, ploty])))])
    track_points = np.hstack((left, right))[0]
    
    color_img = highlightTrack(color_img, track_points, y_warning, y_danger, d_warning, d_danger)

    return (color_img, track_points)

             