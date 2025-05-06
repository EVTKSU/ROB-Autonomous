import cv2
import depthai as dai
import numpy as np

# Path to your downloaded YOLOv8n COCO blob
NN_BLOB_PATH = r"C:\Users\Ben\Downloads\yolov8n_coco_640x352.blob"

def main():
    # 1) Create pipeline
    pipeline = dai.Pipeline()

    # 2) Color camera setup
    camRgb = pipeline.create(dai.node.ColorCamera)
    camRgb.setPreviewSize(640, 352)                                                       # match blob input
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)           # valid AR0234 setting
    camRgb.setInterleaved(False)
    camRgb.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)

    # 3) YOLOv8n detection network
    detectionNetwork = pipeline.create(dai.node.YoloDetectionNetwork)
    detectionNetwork.setBlobPath(NN_BLOB_PATH)
    detectionNetwork.setConfidenceThreshold(0.5)  # filter weak detections
    detectionNetwork.setNumClasses(80)            # COCO classes
    detectionNetwork.setCoordinateSize(4)         # (x, y, w, h)
    detectionNetwork.setIouThreshold(0.5)         # NMS IoU threshold
    detectionNetwork.setNumInferenceThreads(2)
    detectionNetwork.input.setBlocking(False)

    # 4) Link camera preview to detector input
    camRgb.preview.link(detectionNetwork.input)

    # 5) Create outputs for preview and detections
    xoutRgb = pipeline.create(dai.node.XLinkOut)
    xoutRgb.setStreamName("rgb")
    camRgb.preview.link(xoutRgb.input)

    xoutDet = pipeline.create(dai.node.XLinkOut)
    xoutDet.setStreamName("det")
    detectionNetwork.out.link(xoutDet.input)

    # 6) Start device and get queues
    with dai.Device(pipeline) as device:
        qRgb = device.getOutputQueue(name="rgb", maxSize=4, blocking=False)
        qDet = device.getOutputQueue(name="det", maxSize=4, blocking=False)

        cv2.namedWindow("RGB", cv2.WINDOW_NORMAL)

        while True:
            inRgb = qRgb.get()
            frame = inRgb.getCvFrame()

            inDet = qDet.tryGet()
            if inDet:
                for detection in inDet.detections:
                    # Scale normalized bbox to pixel coords
                    x1 = int(detection.xmin * frame.shape[1])
                    y1 = int(detection.ymin * frame.shape[0])
                    x2 = int(detection.xmax * frame.shape[1])
                    y2 = int(detection.ymax * frame.shape[0])
                    # Draw green box
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

            cv2.imshow("RGB", frame)
            if cv2.waitKey(1) == ord('q'):
                break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
