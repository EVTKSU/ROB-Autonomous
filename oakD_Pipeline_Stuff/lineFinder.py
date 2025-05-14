import cv2
import depthai as dai
import numpy as np

def create_pipeline():
    pipeline = dai.Pipeline()
    # 1) Color camera → ImageManip (BGR888p, 640×352)
    cam = pipeline.create(dai.node.ColorCamera)
    cam.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P)
    cam.setInterleaved(False)
    cam.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)

    manip = pipeline.create(dai.node.ImageManip)
    manip.initialConfig.setFrameType(dai.ImgFrame.Type.BGR888p)  # planar :contentReference[oaicite:6]{index=6}
    manip.initialConfig.setResize(640, 352)
    manip.initialConfig.setKeepAspectRatio(False)
    cam.video.link(manip.inputImage)

    # 2) XLink out to host
    xout = pipeline.create(dai.node.XLinkOut)
    xout.setStreamName("video")
    manip.out.link(xout.input)

    return pipeline

def process_frame(frame):
    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)             # to single channel
    # Threshold for bright (white) regions
    _, thresh = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY)  # isolate white lines :contentReference[oaicite:7]{index=7}
    # Blur and Canny
    blurred = cv2.GaussianBlur(thresh, (5,5), 0)
    edges = cv2.Canny(blurred, 50, 150)                        # edge detection :contentReference[oaicite:8]{index=8}

    # HoughLinesP to find line segments
    lines = cv2.HoughLinesP(edges,
                            rho=1,
                            theta=np.pi/180,
                            threshold=50,
                            minLineLength=100,
                            maxLineGap=10)                   # probabilistic Hough :contentReference[oaicite:9]{index=9}
    if lines is None:
        return frame

    height, width = frame.shape[:2]
    left_x, right_x = [], []
    for x1, y1, x2, y2 in lines[:,0]:
        # Compute slope
        if x2 == x1: 
            slope = np.inf
        else:
            slope = (y2 - y1) / (x2 - x1)
        # Filter near-vertical lines
        if abs(slope) > 1.0:
            # Compute x-intercept at bottom (y=height)
            x_bottom = int(x1 + (height - y1) * (x2 - x1) / (y2 - y1))
            # Classify left vs right half
            if x_bottom < width // 2:
                left_x.append(x_bottom)
            else:
                right_x.append(x_bottom)
            # Optionally draw the segment
            cv2.line(frame, (x1,y1), (x2,y2), (0,255,0), 2)

    # Compute average bottom intercepts
    if left_x and right_x:
        avg_left  = int(np.mean(left_x))
        avg_right = int(np.mean(right_x))
        midpoint  = (avg_left + avg_right) // 2
        # Draw red circle at midpoint, centered vertically
        cv2.circle(frame, (midpoint, height//2), 8, (0,0,255), -1)  # marker :contentReference[oaicite:10]{index=10}

    return frame

def main():
    pipeline = create_pipeline()
    with dai.Device(pipeline, dai.DeviceInfo("169.254.1.222")) as device:
        q = device.getOutputQueue(name="video", maxSize=4, blocking=False)
        cv2.namedWindow("Track Center", cv2.WINDOW_NORMAL)

        while True:
            inFrame = q.get()  
            frame = inFrame.getCvFrame()
            out = process_frame(frame)
            cv2.imshow("Track Center", out)
            if cv2.waitKey(1) == ord('q'):
                break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
