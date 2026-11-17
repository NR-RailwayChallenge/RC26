# -*- coding: utf-8 -*-
"""
main.py

Main file, where parameters can be tweaked and then run the code

Created on Wed Apr  1 00:47:07 2026

@author: KTam1
"""

### Data Inputs
# Image: If you want to run the code on images
image_bool = False
image_file = 'RC25Photo/TrackImage14.png'

# Video: If you want to run the code on a video stream
video_bool = False
video_file = 'RC25Photo/Cropped_RC25_AiVR.mp4'
output_vid = 'outputtrial.mp4'

camera_bool = True

### Parameters Tweaking
# Region of interest: How much margin to give (i.e., half base of triangle)
# For the mask
region_margin = 400
max_height = 200 #200 # Max height for AI camera is 480!!!!, default for original camera is 500

# Probabilistic Hough
prob_threshold = 70 # Confidence that it is a true line
prob_line_length = 30 # Minimum length of the lines, in pixels
prob_line_gap = 10 # Distance away from each line, in pixels

# Specify the width of track expected (pixels, along the bottom of the picture)
min_width = 100 #100 # 200
max_width = 200 #200 # 400

# Warning System
a_decel = -1.0 # Deceleration in m/s^2
t_warning = 8.0 # Hazard warning, impact in how many seconds
t_danger = 3.0 # Danger warning, impact in how many seconds
safety_factor = 1.5 # Multiply warning distances by how much
current_velocity = 0.5 # m/s, will change to dynamic input from loco
detect_what = ["person"] # What category to detect

# Specify reference points
camera_height = 1.0#1.0 # Height above tracks, in metres

### Whenever this is run, it will run the main python script
#with open("directory.py") as file:
#    exec(file.read())

### Function modules
import cv2
import BrakingDistance # Converts image into estimated distances
import TrackDetect # Load Track Detection functoins

import numpy as np

### Image Recognition Inputs
if (image_bool == True) | (video_bool == True):
    import YOLOLoad # Load YOLO packages
    import LocalImage # Create images (local model)
    import LocalVideo # Create videos (local model)
    yolo = YOLOLoad.YOLO_init()
    
###### Functions
"""
common_pipeline

Code which is common to all pipelines

Input: An image

Outputs:
- color_img: Image with track shaded
- mask: Edge detected
- track_points: coordinates of the tracks
- y_warning: Height below the top of image when warning should be given (pixels)
- y_danger: Height below the top of image when danger should be given (pixels)
"""
def common_pipeline(img):    
    # Get key dimensions of image
    img = np.copy(img)

    # Detect the track using edge detection
    (mask, curves, lanes, ploty, success) = TrackDetect.main(img, region_margin, max_height, prob_threshold, prob_line_length, prob_line_gap, min_width, max_width)
    
    # Get the braking distance y-values
    (y_warning, y_danger, d_warning, d_danger) = BrakingDistance.getWarningHeights(current_velocity, a_decel, t_warning, t_danger, safety_factor, camera_height, img.shape[0])
    
    # Highlight the track with braking distances
    (color_img, track_points) = BrakingDistance.drawTrack(img, ploty, curves[0], curves[1], y_warning, y_danger, d_warning, d_danger)

    return (color_img, mask, track_points, y_warning, y_danger)

"""
LocalImage_pipeline

Function which runs the threat detection algorithm for a local image

Input: An image
Outputs:
    full_img: An annotated image
    mask: Edge detected
"""
def LocalImage_pipeline(img):
    (color_img, mask, track_points, y_warning, y_danger) = common_pipeline(img)

    # Classification and threat detection
    (color_img, warning_count, danger_count) = YOLOLoad.YOLO_HighlightObject(yolo, img, color_img, track_points, y_warning, y_danger, detect_what)

    # Overlay original imgae with colour image (lower opacity) to overlay detection onto image
    full_img = cv2.addWeighted(img, 1, color_img, 0.7, 0)

    return (full_img, mask)

"""
LocalVideo_pipeline

Function which runs the threat detection algorithm for a local video

Input: An image
Outputs: An annotated image
"""
def LocalVideo_pipeline(img):
    (color_img, _, track_points, y_warning, y_danger) = common_pipeline(img)

    # Classification and threat detection
    (color_img, warning_count, danger_count) = YOLOLoad.YOLO_HighlightObject(yolo, img, color_img, track_points, y_warning, y_danger, detect_what)

    # Overlay original imgae with colour image (lower opacity) to overlay detection onto image
    full_img = cv2.addWeighted(img, 1, color_img, 0.7, 0)

    return full_img

### Actual Main Code
## Image Processing
if image_bool == True:
    img = cv2.imread(image_file)
    (img_, dst) = LocalImage_pipeline(img)
    LocalImage.ImagePlot(img, dst, img_)

## Video Processing
if video_bool == True:
    LocalVideo.RunVideo(LocalVideo_pipeline, video_file, output_vid) #RunVideo_parallel

if camera_bool == True:
    print("Parameters Loaded Successfully")
    #camera_on = True
    #with open("CameraFeed.py") as file:
    #    exec(file.read())
