import cv2
import depthai as dai
import numpy as np

def process_frame(frame):
    """
    Convert the frame to grayscale, threshold to isolate white areas,
    and use morphological operations to reduce noise.
    """
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    # Threshold the grayscale image to capture the white line; adjust threshold as needed.
    _, mask = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY)
    kernel = np.ones((5, 5), np.uint8)
    mask = cv2.erode(mask, kernel, iterations=1)
    mask = cv2.dilate(mask, kernel, iterations=2)
    return mask

def find_line(mask, frame):
    """
    Find contours on the mask and return the center (cx, cy) of the largest contour,
    assumed to be the white line.
    """
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        M = cv2.moments(largest_contour)
        if M["m00"] > 0:
            cx = int(M["m10"] / M["m00"])
            cy = int(M["m01"] / M["m00"])
            cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
            return cx, cy
    return None, None

def get_depth_at_pixel(depthFrame, cx, cy):
    """
    Retrieve the depth value at pixel (cx, cy) by averaging over a small window.
    Depth values are in meters.
    """
    h, w = depthFrame.shape
    if cx < 0 or cy < 0 or cx >= w or cy >= h:
        return 0
    window_size = 5
    half_ws = window_size // 2
    x0 = max(cx - half_ws, 0)
    y0 = max(cy - half_ws, 0)
    x1 = min(cx + half_ws, w - 1)
    y1 = min(cy + half_ws, h - 1)
    roi = depthFrame[y0:y1+1, x0:x1+1]
    avg_depth = np.mean(roi)
    return avg_depth

def main():
    # Create a pipeline object
    pipeline = dai.Pipeline()

    # ---------- Pipeline Setup ----------
    # Create a color camera node using CAM_A (avoiding the deprecated RGB socket)
    camRgb = pipeline.createColorCamera()
    camRgb.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)
    camRgb.setInterleaved(False)  # Recommended for best performance
    
    # Use the video output (as in your working code)
    xoutRgb = pipeline.createXLinkOut()
    xoutRgb.setStreamName("rgb")
    camRgb.video.link(xoutRgb.input)
    
    # ---------- Add Stereo Depth Nodes ----------
    # Create mono cameras for stereo depth computation
    monoLeft = pipeline.createMonoCamera()
    monoLeft.setBoardSocket(dai.CameraBoardSocket.LEFT)
    monoLeft.setResolution(dai.MonoCameraProperties.SensorResolution.THE_400_P)
    
    monoRight = pipeline.createMonoCamera()
    monoRight.setBoardSocket(dai.CameraBoardSocket.RIGHT)
    monoRight.setResolution(dai.MonoCameraProperties.SensorResolution.THE_400_P)
    
    # Create and configure the stereo depth node
    stereo = pipeline.createStereoDepth()
    stereo.setDefaultProfilePreset(dai.node.StereoDepth.PresetMode.HIGH_DENSITY)
    monoLeft.out.link(stereo.left)
    monoRight.out.link(stereo.right)
    
    # Create an output stream for the depth data
    xoutDepth = pipeline.createXLinkOut()
    xoutDepth.setStreamName("depth")
    stereo.depth.link(xoutDepth.input)
    
    # ---------- Ethernet Connection Setup ----------
    # Replace with the actual IP address assigned to your OAK-D-LR
    device_ip = "169.254.1.222"  
    device_info = dai.DeviceInfo(device_ip)
    
    # Create the windows before connecting to the device
    cv2.namedWindow("RGB", cv2.WINDOW_NORMAL)
    cv2.namedWindow("Depth", cv2.WINDOW_NORMAL)
    cv2.namedWindow("Mask", cv2.WINDOW_NORMAL)

    # Calibration parameter: focal length in pixels (this example assumes 300 pixels; adjust per your calibration)
    fx = 300.0

    # ---------- Connect to the Device ----------
    try:
        with dai.Device(pipeline, device_info) as device:
            print("Connected to device via Ethernet!")
            # Obtain the output queues for video and depth streams.
            qRgb = device.getOutputQueue(name="rgb", maxSize=4, blocking=False)
            qDepth = device.getOutputQueue(name="depth", maxSize=4, blocking=False)

            while True:
                # Break the loop if the window is closed.
                if cv2.getWindowProperty("RGB", cv2.WND_PROP_VISIBLE) < 1:
                    break

                # Get the next available RGB frame.
                inRgb = qRgb.get()
                frame = inRgb.getCvFrame()
                
                # Get the next available depth frame.
                inDepth = qDepth.tryGet()
                if inDepth is not None:
                    depthFrame = inDepth.getFrame()  # depth in millimeters
                    depthFrame = depthFrame.astype(np.float32) / 1000.0  # convert to meters
                else:
                    # In case depth data isn't available.
                    depthFrame = np.zeros((frame.shape[0], frame.shape[1]), dtype=np.float32)
                
                # Process the frame to detect the white line.
                mask = process_frame(frame)
                cx, cy = find_line(mask, frame)
                
                # Calculate the horizontal distance from the camera center
                image_center = frame.shape[1] // 2
                if cx is not None:
                    depth_value = get_depth_at_pixel(depthFrame, cx, cy)
                    # Compute the lateral distance using the pinhole camera model.
                    lateral_distance = (cx - image_center) * depth_value / fx
                    text = f"LatDist: {lateral_distance:.2f} m, Depth: {depth_value:.2f} m"
                    cv2.putText(frame, text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
                    print(text)
                else:
                    cv2.putText(frame, "Line not detected", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                    print("Line not detected")
                
                # Display the frames and processed masks.
                cv2.imshow("RGB", frame)
                cv2.imshow("Mask", mask)
                depthVis = cv2.normalize(depthFrame, None, 0, 255, cv2.NORM_MINMAX)
                cv2.imshow("Depth", np.uint8(depthVis))
                
                # Exit when 'q' is pressed.
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