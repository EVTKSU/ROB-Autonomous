#!/usr/bin/env python3
import os
import sys
import argparse
import cv2
import depthai as dai
import numpy as np

def parse_args():
    p = argparse.ArgumentParser(description="Density‑Based Diagonal Line Finder")
    p.add_argument(
        "--ip", "-i",
        default=os.getenv("OAKD_IP", None),
        help="OAK‑D‑LR IP for Ethernet (omit for USB)"
    )
    p.add_argument(
        "--white-level", "-w",
        type=float,
        default=30.0,
        help="Minimum white level %% (0–100) to include"
    )
    return p.parse_args()

def create_pipeline():
    p = dai.Pipeline()
    cam = p.create(dai.node.ColorCamera)
    cam.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P)
    cam.setInterleaved(False)
    cam.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)
    manip = p.create(dai.node.ImageManip)
    manip.initialConfig.setFrameType(dai.ImgFrame.Type.BGR888p)
    manip.initialConfig.setResize(640, 352)
    manip.initialConfig.setKeepAspectRatio(False)
    cam.video.link(manip.inputImage)
    xout = p.create(dai.node.XLinkOut)
    xout.setStreamName("video")
    manip.out.link(xout.input)
    return p

def process_frame(frame, white_pct, y_limit):
    h, w = frame.shape[:2]

    # 1) Mask out green (grass)
    hls = cv2.cvtColor(frame, cv2.COLOR_BGR2HLS)
    green_mask = cv2.inRange(hls, (35,100,50), (85,255,255))

    # 2) Grayscale + white threshold
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    tv = int(np.clip(white_pct,0,100)/100*255)
    _, mask = cv2.threshold(gray, tv, 255, cv2.THRESH_BINARY)

    # 3) Remove green from mask
    mask[green_mask>0] = 0

    # 4) Ignore above horizontal limit
    mask[:y_limit, :] = 0
    cv2.line(frame, (0,y_limit), (w,y_limit), (255,0,0), 2)

    # 5) Cleanup
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(5,5))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)

    # 6) Debug: red dots at centroids
    contours,_ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    for cnt in contours:
        if cv2.contourArea(cnt) < 50: continue
        M = cv2.moments(cnt)
        if M["m00"] == 0: continue
        cx = int(M["m10"]/M["m00"])
        cy = int(M["m01"]/M["m00"])
        if cy >= y_limit:
            cv2.circle(frame, (cx,cy), 3, (0,0,255), -1)

    # 7) Canny on mask
    edges = cv2.Canny(mask, 50, 150)

    # 8) Standard HoughLines → (ρ,θ) peaks => dense lines
    lines = cv2.HoughLines(edges, 1, np.pi/180, 30)
    if lines is not None:
        for rho,theta in lines[:,0]:
            a = np.cos(theta); b = np.sin(theta)
            x0 = a*rho; y0 = b*rho
            pt1 = (int(x0 + 1000*(-b)), int(y0 + 1000*(a)))
            pt2 = (int(x0 - 1000*(-b)), int(y0 - 1000*(a)))
            # filter near‑horizontal or vertical by slope
            if abs(b) < 0.1: 
                continue
            cv2.line(frame, pt1, pt2, (0,255,0), 2)

    return frame

def main():
    args = parse_args()
    pipeline = create_pipeline()
    try:
        if args.ip:
            device = dai.Device(pipeline, dai.DeviceInfo(args.ip))
        else:
            device = dai.Device(pipeline)
    except RuntimeError as e:
        print(f"[ERROR] Connection failed: {e}")
        sys.exit(1)

    q = device.getOutputQueue("video", maxSize=4, blocking=False)
    win = "Density Line Finder"
    cv2.namedWindow(win, cv2.WINDOW_NORMAL)
    cv2.createTrackbar("Y Limit", win, 176, 352, lambda x:None)
    cv2.createTrackbar("White %", win, int(args.white_level), 100, lambda x:None)

    while True:
        inF = q.tryGet()
        if inF:
            frm = inF.getCvFrame()
            y_lim = cv2.getTrackbarPos("Y Limit", win)
            w_pct = cv2.getTrackbarPos("White %", win)
            out = process_frame(frm, w_pct, y_lim)
            cv2.imshow(win, out)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
