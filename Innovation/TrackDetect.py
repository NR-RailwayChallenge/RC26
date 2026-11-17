# -*- coding: utf-8 -*-
"""
TrackDetect.py

Functions which relate to the detection of the track

Created on Tue Mar 31 22:55:47 2026

@author: KTam1
"""

### Import Modules
import cv2
#from skimage import img_as_ubyte, feature
from skimage.transform import probabilistic_hough_line
import numpy as np
#import matplotlib.pyplot as plt

def line_prop(coor):
    # Coordinates for first two points
    x1,y1 = coor[0]
    x2,y2 = coor[1]
    
    # Ignore data which is a vertical line (avoid issues with vertical poles)
    if x2 == x1:
        m = np.nan
        c = np.nan
    # Otherwise, calculate the gradient and y-intercepts
    else:  
        m = (y2-y1)/(x2-x1)
        c = y2 - m*x2

    return (m, c)

# get_x: Get the x value on a line
def get_x(image_height,m,c):
    return (image_height - c)/m

# Region of Interest: Filter out irrelevant details
def region_of_interest(image, mid_point, margin = 500, max_height=500):
    # Obtain key geometries of the image
    height, width = image.shape
    
    mid_point = int(mid_point)
    
    # Get the coordinates of the triangle
    r = np.array([height, height, max_height, max_height])   # y-coordinates
    c = np.array([mid_point-margin, mid_point+margin, mid_point+margin, mid_point-margin]) #mid_point+100, mid_point-100])   # x-coordinates
    
    # ---- 3. Compute bounding box (key optimization) ----
    rmin, rmax = r.min(), r.max()
    cmin, cmax = c.min(), c.max()
    
    rmin = max(rmin, 0)
    cmin = max(cmin, 0)
    rmax = min(rmax, height - 1)
    cmax = min(cmax, width - 1)
    
    # ---- 4. Extract ROI (cropped image) ----
    roi = image[rmin:rmax+1, cmin:cmax+1]

    # Canny edge detection
    blurred = cv2.GaussianBlur(roi, (0, 0), sigmaX=4, sigmaY=4)
    edges_roi = cv2.Canny(blurred, 25, 70, 3)
    
    # ---- 5. Build mask ONLY for ROI ----
    mask = np.zeros(roi.shape, dtype=np.uint8)
    pts = np.stack([c - cmin, r - rmin], axis=1)
    
    cv2.fillPoly(mask, [pts], 255)
    
    # ---- 6. Apply mask on Canny ----
    masked_result = cv2.bitwise_and(edges_roi, edges_roi, mask=mask)

    return (masked_result, rmin, cmin, rmax, cmax)

left_a, left_b, left_c = [],[],[]
right_a, right_b, right_c = [],[],[]

# sliding_window: Create a sliding window which gives the best guess of where the rails are
def sliding_window(img, success, rail_left_m, rail_left_c, rail_right_m, rail_right_c, nwindows=15, margin=30, max_height=300, minpix = 1, threshold = 1):
    global left_a, left_b, left_c, right_a, right_b, right_c 
    
    img_height = img.shape[0]
    
    # Initialise values
    left_fit_= np.empty(3)
    right_fit_ = np.empty(3)
    
    # Plot y values
    ploty = np.linspace(max_height, img.shape[0]-1, img.shape[0]-max_height)

    if success == True:
        leftx_base = get_x(img_height,rail_left_m,rail_left_c)
        rightx_base = get_x(img_height,rail_right_m,rail_right_c)

        # Set height of windows
        window_height = np.int64((img_height-max_height)/nwindows)
        
        # Identify the x and y positions of all nonzero pixels in the image
        nonzero = img.nonzero()
        nonzeroy = np.array(nonzero[0])
        nonzerox = np.array(nonzero[1])
        
        # Current positions to be updated for each window
        leftx_current = leftx_base
        rightx_current = rightx_base
        original_width = rightx_base-leftx_base
    
        # Create empty lists to receive left and right lane pixel indices
        left_lane_inds = []
        right_lane_inds = []

        # Step through the windows one by one
        for window in range(nwindows):
            # Identify window boundaries in x and y (and right and left)
            win_y_low = img.shape[0] - (window+1)*window_height
            win_y_high = img.shape[0] - window*window_height
            win_xleft_low = leftx_current - margin
            win_xleft_high = leftx_current + margin
            win_xright_low = rightx_current - margin
            win_xright_high = rightx_current + margin
    
            # Identify the nonzero pixels in x and y within the window
            good_left_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & 
            (nonzerox >= win_xleft_low) &  (nonzerox < win_xleft_high)).nonzero()[0]
            good_right_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & 
            (nonzerox >= win_xright_low) &  (nonzerox < win_xright_high)).nonzero()[0]

            # If you found > minpix pixels, recenter next window on their mean position
            if len(good_left_inds) > minpix:
                # Append these indices to the lists
                left_lane_inds.append(good_left_inds)
                left_new = np.int64(np.mean(nonzerox[good_left_inds]))
                #cv2.rectangle(img,(int(win_xleft_low),int(win_y_low)),(int(win_xleft_high),int(win_y_high)),(100,255,255), 3)
                leftx_current = left_new
            if len(good_right_inds) > minpix:       
                right_lane_inds.append(good_right_inds)
                right_new = np.int64(np.mean(nonzerox[good_right_inds]))
                #cv2.rectangle(img,(int(win_xright_low),int(win_y_low)),(int(win_xright_high),int(win_y_high)),(100,255,255), 3)
                rightx_current = right_new

        # Concatenate the arrays of indices
        left_lane_inds = np.concatenate(left_lane_inds)
        right_lane_inds = np.concatenate(right_lane_inds)
        
        # Default to straight line
        a_temp_left = 0.0
        b_temp_left = rail_left_m
        c_temp_left = rail_left_c
        a_temp_right= 0.0
        b_temp_right = rail_right_m
        c_temp_right = rail_right_c
    
        if (left_lane_inds.size != 0):
            leftx = nonzerox[left_lane_inds]
            lefty = nonzeroy[left_lane_inds] 
            
            # Fit a second order polynomial
            left_fit = np.polyfit(lefty, leftx, 2)

            a_temp_left = left_fit[0]
            b_temp_left = left_fit[1]
            c_temp_left = left_fit[2]
            
        if (right_lane_inds.size != 0):
            # Extract right line pixel positions
            rightx = nonzerox[right_lane_inds]
            righty = nonzeroy[right_lane_inds] 
            
            # Fit a second order polynomial
            right_fit = np.polyfit(righty, rightx, 2)
            
            a_temp_right = right_fit[0]
            b_temp_right = right_fit[1]
            c_temp_right = right_fit[2]
        
        x_left = a_temp_left*max_height**2 + b_temp_left*max_height + c_temp_left
        x_right = a_temp_right*max_height**2 + b_temp_right*max_height + c_temp_right

        # If width is smaller at the end, reliable, append
        if abs(x_right - x_left) < original_width:
            # Add to the list
            left_a.append(a_temp_left)
            left_b.append(b_temp_left)
            left_c.append(c_temp_left)
            right_a.append(a_temp_right)
            right_b.append(b_temp_right)
            right_c.append(c_temp_right)

        left_fit_[0] = np.mean(left_a[-10:])
        left_fit_[1] = np.mean(left_b[-10:])
        left_fit_[2] = np.mean(left_c[-10:])
        
        right_fit_[0] = np.mean(right_a[-10:])
        right_fit_[1] = np.mean(right_b[-10:])
        right_fit_[2] = np.mean(right_c[-10:])
    else:
        # If it was not successful, return giberish
        # It should give a negative x value, so the detection will never be
        # Triggered
        left_fit_ = [-1.0, -1.0, -1.0]
        right_fit_ = [-1.0, -1.0, -1.0]
        
    # Generate x values for plotting
    left_fitx = left_fit_[0]*ploty**2 + left_fit_[1]*ploty + left_fit_[2]
    right_fitx = right_fit_[0]*ploty**2 + right_fit_[1]*ploty + right_fit_[2]

    return (left_fitx, right_fitx), (left_fit_, right_fit_), ploty, img

def main(img, region_margin, max_height, prob_threshold, prob_line_length, prob_line_gap, min_width, max_width):
    # Get image dimensions
    img_width = img.shape[1]
    img_height = img.shape[0]
    mid_point_est = img_width/2
    
    
    # Visualize undistortion
    #f, (ax1, ax2, ax3, ax4) = plt.subplots(1, 4, figsize=(20,10))

    lower_red = np.array([90, 90, 90])
    upper_red = np.array([200, 200, 200])
    mask1 = cv2.inRange(img, lower_red, upper_red)

    result = cv2.bitwise_and(img, img, mask=mask1)
    
    # Greyscale
    grey = cv2.cvtColor(result, cv2.COLOR_RGB2GRAY)
    #ax1.imshow(grey, cmap='gray')
    #ax1.set_title('Greyscale Image', fontsize=30)

    # Region of Interest and Canny Edge Detection
    filtered_roi, rmin, cmin, rmax, cmax = region_of_interest(grey, mid_point=mid_point_est,margin=region_margin, max_height=max_height) # Filter out irrelevant sides by focusing on a triangular area

    # ---- 8. Put result back (optional) ----
    filtered_edge = np.zeros_like(grey)
    filtered_edge[rmin:rmax+1, cmin:cmax+1] = filtered_roi

    #edges = feature.canny(grey, sigma=4)

    #ax2.imshow(filtered_edge)
    #ax2.set_title('Edge Image', fontsize=30)
    
    #ax3.imshow(filtered_edge)
    #ax3.set_title('Filtered Image', fontsize=30)
    
    ### Main while loop to check that the straight line fit is genuine!
    iteration = 0
    success = False
    
    # Number of attempts before giving up
    while iteration < 10:  
        # Find candidate lines using Probabilistic Hough
        tested_angles = np.linspace(-np.pi / 6, np.pi / 6, 90, endpoint=False)  # Only consider straight lines between -30 to 30 deg
        lines = probabilistic_hough_line(filtered_edge, threshold=prob_threshold, line_length=prob_line_length, line_gap=prob_line_gap, theta=tested_angles) # Obtain lines found by the function

        ### Step 3: Linear fit track position
        # Initialise key variables to save gradient and y-intercept details
        save_m = []
        save_c = []
        
        # Obtain the details for each line found
        for coor in lines:
            x1, y1 = coor[0]
            x2, y2 = coor[1]
            (m, c) = line_prop(coor)
            
            # Only save data which are not nan
            if ~np.isnan(m):
                save_m.append(m)
                save_c.append(c)
        
        # At this point there should be two dominant lines, use mean to split into two
        full_mean = np.mean(save_c) # Roughly the midpoint
        
        # Then split into two rails, note it is expected that the two lines cross-over,
        # so y-intercept for left rail would be higher than right
        rail_left_idx = [i for i, x in enumerate(save_c) if x >= full_mean]
        rail_right_idx = [i for i, x in enumerate(save_c) if x < full_mean]
        
        # Convert to np arrays
        save_c = np.array(save_c, dtype=np.float64)
        save_m = np.array(save_m, dtype=np.float64)

        rail_left_idx = np.array(rail_left_idx, dtype=int)
        rail_right_idx = np.array(rail_right_idx, dtype=int)
        
        # Use median to get an approximate gradient and y-intercept
        # Median to avoid bias from potential noise
        rail_left_m = np.median(save_m[rail_left_idx])
        rail_right_m = np.median(save_m[rail_right_idx])
        rail_left_c = np.median(save_c[rail_left_idx])
        rail_right_c = np.median(save_c[rail_right_idx])
        
        ### Step 4: Checks to see if the line fit is reasonable
        # Check 1: If width is negative, something is wrong!
        # Check 2: If the width between the rails is not within a margin at the bottom of the image, then it may not be accurate
        bottom_x_intercept_left = get_x(img_height,rail_left_m,rail_left_c)
        bottom_x_intercept_right = get_x(img_height,rail_right_m,rail_right_c)
        width = bottom_x_intercept_right - bottom_x_intercept_left
        pass_test = (width >= min_width) & (width <= max_width)
    
        # Break while loop if it works
        if pass_test == True:
            success = True
            break
        # Otherwise, try lowering the prob_threshold by 10?
        else:
            iteration += 1
            prob_threshold -= 10
            
            # If the threshold is less than or equal to 60 (cannot be too low)
            if prob_threshold < 60:
                prob_threshold = 60
                prob_line_length += 2 # Increase line length instead, to filter out outliers
     
    #ax4.imshow(img)
    #ax4.plot((-rail_left_c/rail_left_m, (img_height - rail_left_c)/rail_left_m), (0, img_height),":")
    #ax4.plot((-rail_right_c/rail_right_m, (img_height - rail_right_c)/rail_right_m), (0, img_height),":")
    # Create a blank mask (0)
    mask = np.zeros((img_height, img_width), dtype=np.uint8)
    
    if success == False:
        print("Did not successfully find track... skipping image")
    else:
        print("Track Detected!")
        cv2.line(mask, (int(bottom_x_intercept_left), img_height), (int(get_x(max_height,rail_left_m,rail_left_c)), max_height), 255, thickness=1)
        cv2.line(mask, (int(bottom_x_intercept_right), img_height), (int(get_x(max_height,rail_right_m,rail_right_c)), max_height), 255, thickness=1)

    for line in lines:
        x1, y1 = line[0]
        x2, y2 = line[1]
        cv2.line(mask, (x1, y1), (x2, y2), 255, thickness=3)

    curves, lanes, ploty, mask = sliding_window(mask, success, rail_left_m, rail_left_c, rail_right_m, rail_right_c, max_height=max_height)

    return (mask, curves, lanes, ploty, success)
