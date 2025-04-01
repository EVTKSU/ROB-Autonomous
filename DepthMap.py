#!/usr/bin/env python3
import depthai as dai
import cv2
import numpy as np

def main():
    # Create a DepthAI pipeline
    pipeline = dai.Pipeline()

    # Create left and right mono cameras for stereo depth
    monoLeft = pipeline.createMonoCamera()
    monoRight = pipeline.createMonoCamera()
    monoLeft.setBoardSocket(dai.CameraBoardSocket.LEFT)
    monoRight.setBoardSocket(dai.CameraBoardSocket.RIGHT)
    monoLeft.setResolution(dai.MonoCameraProperties.SensorResolution.THE_400_P)
    monoRight.setResolution(dai.MonoCameraProperties.SensorResolution.THE_400_P)

    # Create a StereoDepth node to compute depth
    stereo = pipeline.createStereoDepth()
    stereo.setConfidenceThreshold(200)
    monoLeft.out.link(stereo.left)
    monoRight.out.link(stereo.right)

    # Create an XLinkOut node for the depth output
    xoutDepth = pipeline.createXLinkOut()
    xoutDepth.setStreamName("depth")
    stereo.depth.link(xoutDepth.input)

    # Connect to the device and start the pipeline
    with dai.Device(pipeline) as device:
        depthQueue = device.getOutputQueue(name="depth", maxSize=4, blocking=False)
        while True:
            inDepth = depthQueue.get()  # Blocking call to get a depth frame
            depthFrame = inDepth.getFrame()
            
            # Normalize depth data to 0-255 and convert to uint8
            normDepth = cv2.normalize(depthFrame, None, 0, 255, cv2.NORM_MINMAX)
            normDepth = np.uint8(normDepth)
            
            # Apply a color map for visualization
            depthColorMap = cv2.applyColorMap(normDepth, cv2.COLORMAP_JET)
            
            cv2.imshow("Depth Map", depthColorMap)
            if cv2.waitKey(1) == ord('q'):
                break
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
# # • This code captures depth data using stereo cameras and displays a color-mapped depth map.
# # • It uses DepthAI’s pipeline to process stereo images and compute depth information.
# # • The depth data is normalized and color-mapped for better visualization.
# # • The depth map is displayed in a window using OpenCV’s imshow.
# # • The code is designed to run in a loop, continuously updating the depth map until 'q' is pressed.
# # • The pipeline is created and managed using DepthAI’s API, which handles the camera and processing nodes.