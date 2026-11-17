# -*- coding: utf-8 -*-
"""
LocalVideo.py

Creates local video

Created on Tue Mar 31 23:33:30 2026

@author: KTam1
"""
from moviepy.editor import VideoFileClip

def RunVideo(pipeline, video_file, output_vid):
    myclip = VideoFileClip(video_file)
    clip = myclip.fl_image(pipeline)
    clip.write_videofile(output_vid, audio = False)
