#!/usr/bin/env python3
import depthai as dai
import cv2
import numpy as np

def main():
    # Create a DepthAI pipeline for the RGB camera
    pipeline = dai.Pipeline()
    cam = pipeline.createColorCamera()
    cam.setBoardSocket(dai.CameraBoardSocket.RGB)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_720_P)
    
    xout = pipeline.createXLinkOut()
    xout.setStreamName("video")
    cam.video.link(xout.input)
    
    # Set up parameters for blob detection
    params = cv2.SimpleBlobDetector_Params()
    params.filterByArea = True
    params.minArea = 100
    params.filterByCircularity = False
    params.filterByConvexity = False
    params.filterByInertia = False
    params.filterByColor = True
    params.blobColor = 255  # Detecting bright (white-ish) blobs

    detector = cv2.SimpleBlobDetector_create(params)
    
    with dai.Device(pipeline) as device:
        videoQueue = device.getOutputQueue(name="video", maxSize=4, blocking=False)
        while True:
            inFrame = videoQueue.get()
            frame = inFrame.getCvFrame()
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Detect blobs in the grayscale image
            keypoints = detector.detect(gray)
            # Draw detected blobs as red circles with size proportional to the blob area
            im_with_keypoints = cv2.drawKeypoints(frame, keypoints, np.array([]), (0, 0, 255),
                                                  cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
            cv2.imshow("Blob Detection", im_with_keypoints)
            if cv2.waitKey(1) == ord('q'):
                break
        
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
# # • This code captures video from the OAK‑D camera and applies blob detection to identify and highlight blobs in the image.
# # • It uses OpenCV’s SimpleBlobDetector to find and draw keypoints on the detected blobs.
# # • The detected blobs are displayed in a window, and the program runs until 'q' is pressed.
# # • The blob detection parameters can be adjusted to filter blobs based on area, color, and other properties.
# # • The code is designed to work with the DepthAI device and utilizes its pipeline for video capture.
# # • The detected blobs are drawn on the original frame, allowing for real-time visualization of the detection process.
# # • The code is structured to be easily modifiable for different blob detection criteria or camera settings.
# # • The use of OpenCV for image processing and visualization makes it a versatile solution for various computer vision tasks.
# # • The code is intended to be run in a Python environment with DepthAI and OpenCV installed.
# # • The pipeline is created and managed using DepthAI’s API, which handles the camera and processing nodes.