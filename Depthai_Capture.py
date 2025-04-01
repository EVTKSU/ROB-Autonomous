#!/usr/bin/env python3
import depthai as dai
import cv2

def main():
    # Create a DepthAI pipeline
    pipeline = dai.Pipeline()

    # Create a ColorCamera node for capturing RGB frames
    cam = pipeline.createColorCamera()
    cam.setBoardSocket(dai.CameraBoardSocket.RGB)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)

    # Create an XLinkOut node to stream frames to the host
    xout = pipeline.createXLinkOut()
    xout.setStreamName("video")
    cam.video.link(xout.input)

    # Connect to the device and start the pipeline
    with dai.Device(pipeline) as device:
        videoQueue = device.getOutputQueue(name="video", maxSize=4, blocking=False)
        while True:
            inVideo = videoQueue.get()  # Blocking call to get a frame
            frame = inVideo.getCvFrame()
            cv2.imshow("OAK‑D Video", frame)
            if cv2.waitKey(1) == ord('q'):
                break
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()

# • We define and start a pipeline that streams video from the OAK‑D’s RGB camera.
# • The frame is displayed in a window using OpenCV’s imshow.