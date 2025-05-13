#!/usr/bin/env python3
import os
import sys
import cv2
import depthai as dai

def main():
    # 1) Get OAK‑D‑LR IP from env or use default
    ip = os.getenv("OAKD_IP", "192.168.1.177")
    print(f"Connecting to OAK‑D‑LR at {ip} via Ethernet…")

    # 2) Create pipeline
    pipeline = dai.Pipeline()
    cam = pipeline.create(dai.node.ColorCamera)
    cam.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    # AR0234 on OAK‑D‑LR only supports 1200_P
    cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P)
    cam.setInterleaved(False)
    cam.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)
    cam.setPreviewSize(640, 480)
    cam.setPreviewKeepAspectRatio(False)

    # 3) Output the preview to host
    xout = pipeline.create(dai.node.XLinkOut)
    xout.setStreamName("preview")
    cam.preview.link(xout.input)

    # 4) Connect to device over Ethernet
    try:
        device_info = dai.DeviceInfo(ip)
        device = dai.Device(pipeline, device_info)
    except RuntimeError as e:
        print("Failed to connect over Ethernet:", e)
        sys.exit(1)

    # 5) Get output queue and display loop
    q = device.getOutputQueue(name="preview", maxSize=4, blocking=False)
    cv2.namedWindow("Ethernet Preview", cv2.WINDOW_NORMAL)

    while True:
        inFrame = q.tryGet()
        if inFrame:
            frame = inFrame.getCvFrame()
            cv2.imshow("Ethernet Preview", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
