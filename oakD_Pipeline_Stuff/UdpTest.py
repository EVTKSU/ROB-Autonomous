import cv2
import depthai as dai

def main():
    # Create a pipeline object
    pipeline = dai.Pipeline()

    # ---------- Pipeline Setup ----------
    # Create a color camera node using CAM_A (avoiding the deprecated RGB socket)
    camRgb = pipeline.createColorCamera()
    camRgb.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)
    camRgb.setInterleaved(False)  # Recommended for best performance
    
    # Create an output stream for the video
    xoutRgb = pipeline.createXLinkOut()
    xoutRgb.setStreamName("rgb")
    camRgb.video.link(xoutRgb.input)
    
    # ---------- Ethernet Connection Setup ----------
    # Replace with the actual IP address assigned to your OAK-D-LR
    device_ip = "169.254.1.222"  
    
    # Create a DeviceInfo instance using the device's IP address
    device_info = dai.DeviceInfo(device_ip)

    # ---------- Connect to the Device ----------
    # Corrected argument order: pipeline first, then device_info
    with dai.Device(pipeline, device_info) as device:
        print("Connected to device via Ethernet!")
        # Obtain the output queue to stream video frames
        qRgb = device.getOutputQueue(name="rgb", maxSize=4, blocking=False)

        # Continuously fetch and display the frames
        while True:
            inRgb = qRgb.get()  # Get next available frame
            frame = inRgb.getCvFrame()
            cv2.imshow("RGB", frame)

            if cv2.waitKey(1) == ord('q'):
                break

    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
