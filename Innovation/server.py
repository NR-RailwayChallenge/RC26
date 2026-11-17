# -*- coding: utf-8 -*-
"""
server.py

Runs the AI Camera via a Server

Created on Mon Jun  22 08:20:23 2026

@author: KTam1
"""

from flask import Flask, Response, render_template_string
import cv2
import sys
import time
import argparse
from functools import lru_cache
import threading
from picamera2 import MappedArray, Picamera2
from picamera2.devices import IMX500
from picamera2.devices.imx500 import (NetworkIntrinsics,
                                      postprocess_nanodet_detection)

import numpy as np                 
from main import common_pipeline

app = Flask(__name__)

def reset_global_vars():
    global picam2, camera_on, latest_frame, last_results, intrinsics, args, imx500
    picam2 = None
    camera_on = False
    latest_frame = None
    last_results = None
    intrinsics = None
    args = None
    imx500 = None
    

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=str, help="Path of the model",
                        default="/usr/share/imx500-models/imx500_network_ssd_mobilenetv2_fpnlite_320x320_pp.rpk")
    parser.add_argument("--fps", type=int, help="Frames per second")
    parser.add_argument("--bbox-normalization", action=argparse.BooleanOptionalAction, help="Normalize bbox")
    parser.add_argument("--bbox-order", choices=["yx", "xy"], default="yx",
                        help="Set bbox order yx -> (y0, x0, y1, x1) xy -> (x0, y0, x1, y1)")
    parser.add_argument("--threshold", type=float, default=0.55, help="Detection threshold")
    parser.add_argument("--iou", type=float, default=0.65, help="Set iou threshold")
    parser.add_argument("--max-detections", type=int, default=10, help="Set max detections")
    parser.add_argument("--ignore-dash-labels", action=argparse.BooleanOptionalAction, help="Remove '-' labels ")
    parser.add_argument("--postprocess", choices=["", "nanodet"],
                        default=None, help="Run post process of type")
    parser.add_argument("-r", "--preserve-aspect-ratio", action=argparse.BooleanOptionalAction,
                        help="preserve the pixel aspect ratio of the input tensor")
    parser.add_argument("--labels", type=str,
                        help="Path to the labels file")
    parser.add_argument("--print-intrinsics", action="store_true",
                        help="Print JSON network_intrinsics then exit")
    return parser.parse_args()


# Initialise Global Variables
reset_global_vars()

class Detection:
    def __init__(self, coords, category, conf, metadata):
        """Create a Detection object, recording the bounding box, category and confidence."""
        self.category = category
        self.conf = conf
        self.box = imx500.convert_inference_coords(coords, metadata, picam2)

def parse_detections(metadata: dict):
    """Parse the output tensor into a number of detected objects, scaled to the ISP output."""
    global last_detections, intrinsics, args, imx500
    bbox_normalization = intrinsics.bbox_normalization
    bbox_order = intrinsics.bbox_order
    threshold = args.threshold
    iou = args.iou
    max_detections = args.max_detections
    
    np_outputs = imx500.get_outputs(metadata, add_batch=True)
    input_w, input_h = imx500.get_input_size()
    if np_outputs is None:
        return last_detections
    if intrinsics.postprocess == "nanodet":
        boxes, scores, classes = \
            postprocess_nanodet_detection(outputs=np_outputs[0], conf=threshold, iou_thres=iou,
                                          max_out_dets=max_detections)[0]
        from picamera2.devices.imx500.postprocess import scale_boxes
        boxes = scale_boxes(boxes, 1, 1, input_h, input_w, False, False)
    else:
        boxes, scores, classes = np_outputs[0][0], np_outputs[1][0], np_outputs[2][0]
        if bbox_normalization:
            boxes = boxes / input_h

        if bbox_order == "xy":
            boxes = boxes[:, [1, 0, 3, 2]]

    last_detections = [
        Detection(box, category, score, metadata)
        for box, score, category in zip(boxes, scores, classes)
        if score > threshold
    ]

    return last_detections


@lru_cache
def get_labels():
    labels = intrinsics.labels

    if intrinsics.ignore_dash_labels:
        labels = [label for label in labels if label and label != "-"]
    return labels

def camera_pipeline(request, stream="main"):
    global last_results
    """Draw the detections for this request onto the ISP output."""
    
    danger_count = 0
    warning_count = 0
    
    detections = last_results

    with MappedArray(request, stream) as m:
        # Create a copy of the array to draw the background with opacity
        overlay = m.array.copy()

        (color_img, _, track_points, y_warning, y_danger) = common_pipeline(cv2.cvtColor(overlay, cv2.COLOR_RGBA2RGB))

        color_img = cv2.cvtColor(color_img, cv2.COLOR_RGB2RGBA)
        cv2.addWeighted(overlay, 1, color_img, 0.7, 0, m.array)

        if detections is None:
            return

        labels = get_labels()

        for detection in detections:
            x, y, w, h = detection.box
            label = f"{labels[int(detection.category)]} ({detection.conf:.2f})"

            y_max = y + h # Get the maximum y value (i.e., closest to bottom of screen)
            matching_rows = track_points[track_points[:, 1] == y_max]

            colour = (0,255,0)
            lineWidth = 5
            
            if len(matching_rows) > 0:
                first_col = matching_rows[:, 0]
                min_val = np.min(first_col)
                max_val = np.max(first_col)
                min_x = x
                max_x = x + h
                
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

            # Calculate text size and position
            (text_width, text_height), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
            text_x = x + 5
            text_y = y + 15

            # Draw the background rectangle on the overlay
            cv2.rectangle(overlay,
                          (text_x, text_y - text_height),
                          (text_x + text_width, text_y + baseline),
                          (255, 255, 255),  # Background color (white)
                          cv2.FILLED)

            alpha = 0.30
            
            cv2.addWeighted(overlay, alpha, m.array, 1 - alpha, 0, m.array)

            # Draw text on top of the background
            cv2.putText(m.array, label, (text_x, text_y),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)

            # Draw detection box
            cv2.rectangle(m.array, (x, y), (x + w, y + h), colour, lineWidth)

        if intrinsics.preserve_aspect_ratio:
            b_x, b_y, b_w, b_h = imx500.get_roi_scaled(request)
            color = (255, 0, 0)  # red
            cv2.putText(m.array, "ROI", (b_x + 5, b_y + 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
            cv2.rectangle(m.array, (b_x, b_y), (b_x + b_w, b_y + b_h), (255, 0, 0, 0))
        
        global latest_frame
        # Convert RGBA → BGR for OpenCV/web
        frame = cv2.cvtColor(m.array, cv2.COLOR_RGBA2BGR)
        
        # Encode to JPEG
        _, buffer = cv2.imencode('.jpg', frame)

        latest_frame = buffer.tobytes()

def generate():
    global latest_frame, camera_on

    while True:
        if not camera_on:
            time.sleep(0.1)
            continue

        frame = latest_frame

        if frame is None:
            time.sleep(0.01)
            continue

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/')
def index():
    return render_template_string("""
    <html>
    <body>
        <h2>Pi Camera Control</h2>
        <button onclick="start()">Start</button>
        <button onclick="stop()">Stop</button>
        <br><br>
        <img id="feed" width="640">

        <script>
            function start() {
                fetch('/start');
                document.getElementById("feed").src = "/video";
            }

            function stop() {
                fetch('/stop');
                document.getElementById("feed").src = "";
            }
        </script>
    </body>
    </html>
    """)
           
def start_camera():
    global camera_on, picam2, intrinsics, args, imx500, last_results
    
    with open("main.py") as file:
        exec(file.read())

    last_detections = []

    args = get_args()

    # This must be called before instantiation of Picamera2
    imx500 = IMX500(args.model)
    intrinsics = imx500.network_intrinsics

    if not intrinsics:
        intrinsics = NetworkIntrinsics()
        intrinsics.task = "object detection"
    elif intrinsics.task != "object detection":
        print("Network is not an object detection task", file=sys.stderr)
        exit()

    # Override intrinsics from args
    for key, value in vars(args).items():
        if key == 'labels' and value is not None:
            with open(value, 'r') as f:
                intrinsics.labels = f.read().splitlines()
        elif hasattr(intrinsics, key) and value is not None:
            setattr(intrinsics, key, value)

    # Defaults
    if intrinsics.labels is None:
        with open("assets/coco_labels.txt", "r") as f:
            intrinsics.labels = f.read().splitlines()
    intrinsics.update_with_defaults()

    if args.print_intrinsics:
        print(intrinsics)
        exit()

    picam2 = Picamera2(imx500.camera_num)

    #config = picam2.create_preview_configuration(controls={"FrameRate": intrinsics.inference_rate}, buffer_count=2)

    imx500.show_network_fw_progress_bar()

    from picamera2.encoders import H264Encoder
    video_config = picam2.create_video_configuration(controls={"FrameRate": intrinsics.inference_rate}, buffer_count=2)

    picam2.configure(video_config)

    encoder= H264Encoder(10000000)

    picam2.start_and_record_video("test.mp4", show_preview=False)
    
   # picam2.start()
    #picam2.start_recording(encoder, "test.mp4")

    if intrinsics.preserve_aspect_ratio:
        imx500.set_auto_aspect_ratio()

    picam2.post_callback = camera_pipeline

    print("Recording... Press Ctrl+C to stop.")

    try:
        iteration = 0
        while (iteration < 100000) and (camera_on == True): # iteration < 10
            iteration += 1
            print(iteration)
            last_results = parse_detections(picam2.capture_metadata())
    finally:
        print("Releasing camera...")

        if picam2 is not None:
            try:
                picam2.stop_recording()
            except:
                pass

            try:
                picam2.stop()
            except:
                pass

            try:
                picam2.close()
            except:
                pass
            picam2 = None

        print("Camera released")
    #except KeyboardInterrupt:
     #   pass

@app.route('/start')
def start():
    global camera_on, picam2

    if not camera_on:
        camera_on = True
        if picam2 is None:
            threading.Thread(target=start_camera, daemon=True).start()
        
    return "Started"

@app.route('/stop')
def stop():
    global camera_on, picam2

    camera_on = False

    #reset_global_vars()

    return "Stopped"

@app.route('/video')
def video():
    return Response(generate(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000)

