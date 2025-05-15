import cv2
import depthai as dai
import numpy as np
import os

# Path to your YOLOv8n COCO blob
cwd = os.getcwd()
oak_path = "blobs/yolov8n_coco_640x352.blob"
NN_BLOB_PATH = os.path.join(cwd, oak_path)


# COCO class labels (80 total)
labelMap = [
    "person","bicycle","car","motorbike","aeroplane","bus","train","truck","boat",
    "traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat",
    "dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella",
    "handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite",
    "baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle",
    "wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange",
    "broccoli","carrot","hot dog","pizza","donut","cake","chair","sofa","pottedplant","bed",
    "diningtable","toilet","tvmonitor","laptop","mouse","remote","keyboard","cell phone",
    "microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors",
    "teddy bear","hair drier","toothbrush", "bomb"
]

def main():
    # 1) Build pipeline
    pipeline = dai.Pipeline()

    # 2) RGB camera at full resolution
    camRgb = pipeline.create(dai.node.ColorCamera)
    camRgb.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P)
    camRgb.setInterleaved(False)
    camRgb.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)

    # 3) ImageManip for RGB: output planar BGR888p, resized to 640×352
    manipRgb = pipeline.create(dai.node.ImageManip)
    manipRgb.initialConfig.setFrameType(dai.ImgFrame.Type.BGR888p)
    manipRgb.initialConfig.setResize(640, 352)
    manipRgb.initialConfig.setKeepAspectRatio(False)
    camRgb.video.link(manipRgb.inputImage)

    # 4) Mono cameras at THE_1200_P, then downscale via ImageManip to 640×480
    monoLeft  = pipeline.create(dai.node.MonoCamera)
    monoRight = pipeline.create(dai.node.MonoCamera)
    monoLeft.setBoardSocket(dai.CameraBoardSocket.CAM_B)
    monoRight.setBoardSocket(dai.CameraBoardSocket.CAM_C)
    monoLeft.setResolution(dai.MonoCameraProperties.SensorResolution.THE_1200_P)
    monoRight.setResolution(dai.MonoCameraProperties.SensorResolution.THE_1200_P)

    leftManip = pipeline.create(dai.node.ImageManip)
    rightManip = pipeline.create(dai.node.ImageManip)
    for m in (leftManip, rightManip):
        m.initialConfig.setFrameType(dai.ImgFrame.Type.GRAY8)
        m.initialConfig.setResize(640, 480)
        m.initialConfig.setKeepAspectRatio(False)
    monoLeft.out.link(leftManip.inputImage)
    monoRight.out.link(rightManip.inputImage)

    # 5) StereoDepth configured for 640×480 inputs, aligned & capped to 640×352
    stereo = pipeline.create(dai.node.StereoDepth)
    stereo.setInputResolution(640, 480)
    stereo.setDepthAlign(dai.CameraBoardSocket.CAM_A)
    stereo.setOutputSize(640, 352)
    leftManip.out.link(stereo.left)
    rightManip.out.link(stereo.right)

    # 6) Spatial YOLOv8n detection network
    spatialDet = pipeline.create(dai.node.YoloSpatialDetectionNetwork)
    spatialDet.setBlobPath(NN_BLOB_PATH)
    spatialDet.setConfidenceThreshold(0.5)
    spatialDet.setNumClasses(80)
    spatialDet.setCoordinateSize(4)
    spatialDet.setIouThreshold(0.5)
    spatialDet.setBoundingBoxScaleFactor(0.5)
    spatialDet.setNumInferenceThreads(2)
    spatialDet.input.setBlocking(False)
    manipRgb.out.link(spatialDet.input)
    stereo.depth.link(spatialDet.inputDepth)

    # 7) XLink outputs for host
    xoutRgb     = pipeline.create(dai.node.XLinkOut)
    xoutRgb.setStreamName("rgb")
    manipRgb.out.link(xoutRgb.input)

    xoutSpatial = pipeline.create(dai.node.XLinkOut)
    xoutSpatial.setStreamName("spatial")
    spatialDet.out.link(xoutSpatial.input)

    # 8) Start device and processing loop
    with dai.Device(pipeline, dai.DeviceInfo("169.254.1.222")) as device:
        qRgb     = device.getOutputQueue("rgb",     maxSize=4, blocking=False)
        qSpatial = device.getOutputQueue("spatial", maxSize=4, blocking=False)

        cv2.namedWindow("RGB", cv2.WINDOW_NORMAL)
        while True:
            frame     = qRgb.get().getCvFrame()
            inSpatial = qSpatial.tryGet()

            if inSpatial:
                for det in inSpatial.detections:
                    # 2D bounding box
                    x1 = int(det.xmin * frame.shape[1])
                    y1 = int(det.ymin * frame.shape[0])
                    x2 = int(det.xmax * frame.shape[1])
                    y2 = int(det.ymax * frame.shape[0])
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                    # Label + confidence
                    cid = int(det.label)
                    name = labelMap[cid] if 0 <= cid < len(labelMap) else f"CLS {cid}"
                    conf = det.confidence
                    label_txt = f"{name}: {conf:.2f}"

                    # Depth text
                    z_m = det.spatialCoordinates.z / 1000.0
                    depth_txt = f" Z: {z_m:.2f} m"

                    # Combined background
                    (full_w, full_h), _ = cv2.getTextSize(label_txt + depth_txt,
                                                          cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
                    cv2.rectangle(frame,
                                  (x1, y1 - full_h - 6),
                                  (x1 + full_w + 6, y1),
                                  (0, 255, 0),
                                  cv2.FILLED)

                    # Draw label
                    cv2.putText(frame, label_txt, (x1 + 3, y1 - 4),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)
                    # Draw depth to the right
                    label_w = cv2.getTextSize(label_txt, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)[0][0]
                    cv2.putText(frame, depth_txt, (x1 + 3 + label_w, y1 - 4),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)

            cv2.imshow("RGB", frame)
            if cv2.waitKey(1) == ord('q'):
                break

        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
