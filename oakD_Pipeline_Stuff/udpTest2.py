#!/usr/bin/env python3
import os
import sys
import argparse
import cv2
import depthai as dai
import numpy as np

def parse_args():
    p = argparse.ArgumentParser(description="Smoothed Midpoint Line Finder")
    p.add_argument("--ip","-i",default=os.getenv("OAKD_IP",None),
                   help="OAK‑D‑LR IP for Ethernet (omit for USB)")
    p.add_argument("--white-level","-w",type=float,default=30.0,
                   help="White level %% threshold")
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
    manip.initialConfig.setResize(640,352)
    manip.initialConfig.setKeepAspectRatio(False)
    cam.video.link(manip.inputImage)
    xout = p.create(dai.node.XLinkOut)
    xout.setStreamName("video")
    manip.out.link(xout.input)
    return p

def process_frame(frame, white_pct, y_limit):
    h,w = frame.shape[:2]

    # 1) Remove green
    hls = cv2.cvtColor(frame, cv2.COLOR_BGR2HLS)
    green_mask = cv2.inRange(hls,(35,100,50),(85,255,255))

    # 2) White threshold
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    tv = int(np.clip(white_pct,0,100)/100*255)
    _,mask = cv2.threshold(gray,tv,255,cv2.THRESH_BINARY)
    mask[green_mask>0] = 0

    # 3) Ignore above y_limit
    mask[:y_limit,:] = 0
    cv2.line(frame,(0,y_limit),(w,y_limit),(255,0,0),2)

    # 4) Denoise
    kern = cv2.getStructuringElement(cv2.MORPH_RECT,(5,5))
    mask = cv2.morphologyEx(mask,cv2.MORPH_OPEN,kern,iterations=1)

    # 5) Debug dots
    cnts,_ = cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
    for c in cnts:
        if cv2.contourArea(c)<50: continue
        M = cv2.moments(c)
        if M["m00"]==0: continue
        cx = int(M["m10"]/M["m00"]); cy = int(M["m01"]/M["m00"])
        if cy>=y_limit:
            cv2.circle(frame,(cx,cy),4,(0,0,255),-1)

    # 6) Detect segments
    segs = cv2.HoughLinesP(mask,1,np.pi/180,
                          threshold=20,
                          minLineLength=20,
                          maxLineGap=5)
    if segs is None:
        return frame, None

    # 7) Group by left/right and draw
    left, right = [], []
    for x1,y1,x2,y2 in segs[:,0]:
        length = np.hypot(x2-x1,y2-y1)
        midx = (x1+x2)/2
        if midx < w/2:
            left.append((x1,y1,x2,y2,length))
        else:
            right.append((x1,y1,x2,y2,length))

    # draw all in blue
    for grp in (left, right):
        for x1,y1,x2,y2,_ in grp:
            cv2.line(frame,(x1,y1),(x2,y2),(255,0,0),1)

    pts = []
    # pick longest in each
    if left:
        x1,y1,x2,y2,_ = max(left, key=lambda x:x[4])
        cv2.line(frame,(x1,y1),(x2,y2),(0,255,0),2)
        pts.append(((x1+x2)/2, (y1+y2)/2))
    if right:
        x1,y1,x2,y2,_ = max(right, key=lambda x:x[4])
        cv2.line(frame,(x1,y1),(x2,y2),(0,255,0),2)
        pts.append(((x1+x2)/2, (y1+y2)/2))

    # raw midpoint
    if len(pts)==2:
        (mx1,my1),(mx2,my2) = pts
        return frame, ((mx1+mx2)/2, (my1+my2)/2)
    else:
        return frame, None

def main():
    args = parse_args()
    pipeline = create_pipeline()
    try:
        dev = dai.Device(pipeline, dai.DeviceInfo(args.ip)) if args.ip else dai.Device(pipeline)
    except RuntimeError as e:
        print("Connection failed:", e); sys.exit(1)

    q = dev.getOutputQueue("video", maxSize=4, blocking=False)
    win = "Smoothed Midpoint Finder"
    cv2.namedWindow(win, cv2.WINDOW_NORMAL)
    cv2.createTrackbar("Y Limit",     win, 176, 352, lambda x: None)
    cv2.createTrackbar("White Level", win, int(args.white_level), 100, lambda x: None)

    prev_mid = None
    alpha = 0.2  # smoothing factor

    while True:
        inF = q.tryGet()
        if inF:
            frame = inF.getCvFrame()
            y_lim = cv2.getTrackbarPos("Y Limit",     win)
            w_lvl = cv2.getTrackbarPos("White Level", win)
            out, raw_mid = process_frame(frame, w_lvl, y_lim)

            if raw_mid is not None:
                if prev_mid is None:
                    prev_mid = raw_mid
                else:
                    prev_mid = (alpha*raw_mid[0] + (1-alpha)*prev_mid[0],
                                alpha*raw_mid[1] + (1-alpha)*prev_mid[1])
                smx, smy = int(prev_mid[0]), int(prev_mid[1])
                # draw smoothed vertical line and dot
                cv2.line(out, (smx,0), (smx,out.shape[0]), (0,0,255), 2)
                cv2.circle(out, (smx,smy), 6, (0,0,255), -1)

            cv2.imshow(win, out)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cv2.destroyAllWindows()

if __name__=="__main__":
    main()
