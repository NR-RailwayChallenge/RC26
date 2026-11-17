# -*- coding: utf-8 -*-
"""
YOLOLoad.py

Functions which loads the Python YOLO package instead of using the AI camera's
Model

Created on Tue Mar 31 22:35:52 2026

@author: KTam1
"""

from ultralytics import YOLO
import numpy as np
import cv2

def YOLO_init():
    # Initiate the YOLO image recognition pre-trained model
    return YOLO("yolov8n.pt")

def YOLO_HighlightObject(yolo, img, color_img, track_points, y_warning, y_danger, detect_what):
    results = yolo.track(img, stream=True)

    danger_count = 0
    warning_count = 0
        
    for result in results:
        class_names = result.names
    
        for box in result.boxes:
            
            cls = int(box.cls[0])
            class_name = class_names[cls]
            
            # For now, only consider person
            if (box.conf[0] > 0.4) & (class_name in detect_what):
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                y_max = max(y1,y2) # Get the maximum y value (i.e., closest to bottom of screen)
                matching_rows = track_points[track_points[:, 1] == y_max]

                colour = (0,255,0)
                lineWidth = 5
                
                if len(matching_rows) > 0:
                    first_col = matching_rows[:, 0]
                    min_val = np.min(first_col)
                    max_val = np.max(first_col)
                    min_x = min(x1,x2)
                    max_x = max(x1,x2)
                    
                    # Return alright if the bounding box does not touch the tracks
                    if min_x <= max_val and max_x >= min_val:
                        # Check if the y-value is an issue
                        if y_max >= y_danger:
                            colour = (255, 0, 0)
                            lineWidth = 15
                            danger_count += 1
                        elif y_max >= y_warning:
                            colour = (255,151,0)
                            lineWidth = 20
                            warning_count += 1

                conf = float(box.conf[0])

                cv2.rectangle(color_img, (x1, y1), (x2, y2), colour, lineWidth)

                cv2.putText(color_img, f"{class_name} {conf:.2f}",
                            (x1, max(y1 - 10, 20)), cv2.FONT_HERSHEY_SIMPLEX,
                            0.6, colour, 2)

    return (color_img, warning_count, danger_count)