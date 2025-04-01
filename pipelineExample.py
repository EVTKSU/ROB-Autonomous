#!/usr/bin/env python3
import depthai as dai
import cv2
import numpy as np
import time

def main():
    # Create a DepthAI pipeline
    pipeline = dai.Pipeline()

    # Create a ColorCamera node for RGB capture
    cam = pipeline.createColorCamera()
    cam.setBoardSocket(dai.CameraBoardSocket.RGB)
    # Use a high resolution for better detail (1080p)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)
    
    # Use the video output stream from the camera
    xout_video = pipeline.createXLinkOut()
    xout_video.setStreamName("video")
    cam.video.link(xout_video.input)

    # Connect to the device and start the pipeline
    with dai.Device(pipeline) as device:
        videoQueue = device.getOutputQueue(name="video", maxSize=4, blocking=False)
        
        # Initialize variables for recording processed frames at 10fps
        out = None
        last_record_time = time.time()

        while True:
            # Get the latest frame (blocking call)
            inFrame = videoQueue.get()
            frame = inFrame.getCvFrame()

            # Initialize VideoWriter once with frame dimensions
            if out is None:
                frame_width = frame.shape[1]
                frame_height = frame.shape[0]
                fourcc = cv2.VideoWriter_fourcc(*'XVID')
                out = cv2.VideoWriter("tracklines_record.avi", fourcc, 10.0, (frame_width, frame_height))
            
            # --- Step 1: Remove Green Artifacts ---
            # Convert frame to HSV for robust color segmentation
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            # Define lower and upper bounds for green (tweak as necessary)
            lower_green = np.array([35, 40, 40])
            upper_green = np.array([85, 255, 255])
            green_mask = cv2.inRange(hsv, lower_green, upper_green)
            # Invert the green mask to keep non-green areas
            non_green_mask = cv2.bitwise_not(green_mask)
            # Remove green areas by applying the mask
            frame_no_green = cv2.bitwise_and(frame, frame, mask=non_green_mask)

            # --- Step 2: Isolate White-ish Areas ---
            # Convert the non-green image to grayscale
            gray = cv2.cvtColor(frame_no_green, cv2.COLOR_BGR2GRAY)
            # Apply a threshold to isolate high-intensity (white-ish) areas
            ret, white_mask = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY)
            # Optionally, clean up noise with a morphological closing operation
            kernel = np.ones((3,3), np.uint8)
            white_mask = cv2.morphologyEx(white_mask, cv2.MORPH_CLOSE, kernel)

            # --- Step 3: Edge Detection using Canny ---
            # Run Canny edge detection on the white mask
            edges = cv2.Canny(white_mask, 50, 150)

            # --- Step 4: Hough Line Transform to Find Track Edges ---
            # Use probabilistic Hough Transform to detect line segments
            lines = cv2.HoughLinesP(edges, 1, np.pi/180, threshold=50, minLineLength=50, maxLineGap=10)
            if lines is not None:
                for line in lines:
                    x1, y1, x2, y2 = line[0]
                    # Draw detected lines in red on the original frame
                    cv2.line(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
            
            # --- Display Results ---
            cv2.imshow("Original with Track Lines", frame)
            cv2.imshow("Non-Green Image", frame_no_green)
            cv2.imshow("White Mask", white_mask)
            cv2.imshow("Canny Edges", edges)

            # --- Record Processed Frame at ~10fps ---
            current_time = time.time()
            if (current_time - last_record_time) >= 0.1:
                out.write(frame)
                last_record_time = current_time

            # Press 'q' to quit the loop
            if cv2.waitKey(1) == ord('q'):
                break

        # Release VideoWriter and destroy all windows
        if out is not None:
            out.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
