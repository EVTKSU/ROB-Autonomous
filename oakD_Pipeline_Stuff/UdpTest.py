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
    device_info = dai.DeviceInfo(device_ip)

    # Create the window before connecting to the device so that it exists for property checking.
    cv2.namedWindow("RGB", cv2.WINDOW_NORMAL)

    # ---------- Connect to the Device ----------
    try:
        with dai.Device(pipeline, device_info) as device:
            print("Connected to device via Ethernet!")
            # Obtain the output queue to stream video frames.
            qRgb = device.getOutputQueue(name="rgb", maxSize=4, blocking=False)

            while True:
                # Check if the window has been closed by the user.
                if cv2.getWindowProperty("RGB", cv2.WND_PROP_VISIBLE) < 1:
                    break

                # Get the next available frame.
                inRgb = qRgb.get()  
                frame = inRgb.getCvFrame()
                cv2.imshow("RGB", frame)

                # Check if 'q' was pressed to exit.
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

    except KeyboardInterrupt:
        print("Exiting on keyboard interrupt.")
    except Exception as e:
        print("An error occurred:", e)
    finally:
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
