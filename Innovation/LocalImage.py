# -*- coding: utf-8 -*-
"""
LocalImage.py

Provides a summary from an image

Created on Tue Mar 31 23:30:26 2026

@author: KTam1
"""
import matplotlib.pyplot as plt

def ImagePlot(img, dst, img_):
    # Visualize undistortion
    f, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(20,10))
    ax1.imshow(img)
    ax1.set_title('Original Image', fontsize=30)
    ax2.imshow(dst, cmap='gray')
    ax2.set_title('Warped Image', fontsize=30)
    ax3.imshow(img_)
    ax3.set_title('Overlaid to Image', fontsize=30)
    
    plt.show()

