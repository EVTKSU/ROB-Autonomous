#!/usr/bin/env python3
import depthai as dai
import cv2
import numpy as np

def main():
    pipeline = dai.Pipeline()
    
    # Set up the ColorCamera node
    cam = pipeline.createColorCamera()
    cam.setBoardSocket(dai.CameraBoardSocket.RGB)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_720_P)
    
    # Create an output stream for video frames
    xout = pipeline.createXLinkOut()
    xout.setStreamName("video")
    cam.video.link(xout.input)
    
    # Parameters for Lucas-Kanade optical flow
    lk_params = dict(winSize=(15,15),
                     maxLevel=2,
                     criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))
    
    with dai.Device(pipeline) as device:
        videoQueue = device.getOutputQueue(name="video", maxSize=4, blocking=False)
        prev_gray = None
        prev_points = None
        
        while True:
            inFrame = videoQueue.get()  # Grab a new frame
            frame = inFrame.getCvFrame()
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Initialize previous frame and detect features if not set
            if prev_gray is None:
                prev_gray = gray
                prev_points = cv2.goodFeaturesToTrack(prev_gray, maxCorners=100, qualityLevel=0.3, minDistance=7)
                continue

            # Calculate optical flow to track feature points
            next_points, status, error = cv2.calcOpticalFlowPyrLK(prev_gray, gray, prev_points, None, **lk_params)
            
            # Select only good points
            if next_points is not None and prev_points is not None:
                good_new = next_points[status == 1]
                good_old = prev_points[status == 1]
                
                # Draw the motion vectors
                for new, old in zip(good_new, good_old):
                    a, b = new.ravel()
                    c, d = old.ravel()
                    cv2.line(frame, (int(a), int(b)), (int(c), int(d)), (0, 255, 0), 2)
                    cv2.circle(frame, (int(a), int(b)), 3, (0, 0, 255), -1)
            
            cv2.imshow("Optical Flow Tracking", frame)
            
            # Update the previous frame and points for the next iteration
            prev_gray = gray.copy()
            prev_points = good_new.reshape(-1, 1, 2) if next_points is not None else None
            
            if cv2.waitKey(1) == ord('q'):
                break
        
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
# # • This code captures video frames from a DepthAI device and applies optical flow tracking using the Lucas-Kanade method.
# # • It uses DepthAI’s pipeline to process video frames from the RGB camera.
# # • The optical flow algorithm tracks feature points between consecutive frames.
# # • The tracked points are visualized by drawing lines and circles on the video frames.
# # • The video frames are displayed in a window using OpenCV’s imshow.
# # • The code runs in a loop, continuously updating the optical flow visualization until 'q' is pressed.