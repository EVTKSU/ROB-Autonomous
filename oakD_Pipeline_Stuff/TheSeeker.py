import cv2
import numpy as np
import depthai as dai

# Path to your downloaded depth-estimation blob
NN_BLOB_PATH = r"C:\Users\Ben\Desktop\EVT stuff\ROB-Autonomous\blobs\fast_depth_480x640.blob"

def main():
    # 1) Build pipeline
    pipeline = dai.Pipeline()

    # 2) Configure camera and preview size to match blob input (640×480 planar BGR)
    camRgb = pipeline.createColorCamera()
    camRgb.setBoardSocket(dai.CameraBoardSocket.CAM_A)
    camRgb.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P)  # AR0234 only supports 1200_P
    camRgb.setInterleaved(False)
    camRgb.setPreviewSize(640, 480)

    # 3) Depth-estimation NN node
    depth_nn = pipeline.createNeuralNetwork()
    depth_nn.setBlobPath(NN_BLOB_PATH)
    camRgb.preview.link(depth_nn.input)

    # 4) Outputs: full‑res RGB for display, depth tensor for printing
    xoutRgb   = pipeline.createXLinkOut()
    xoutRgb.setStreamName("rgb")
    camRgb.video.link(xoutRgb.input)

    xoutDepth = pipeline.createXLinkOut()
    xoutDepth.setStreamName("depth")
    depth_nn.out.link(xoutDepth.input)

    # 5) Connect to first available OAK device
    devs = dai.Device.getAllAvailableDevices()
    if not devs:
        print("No OAK devices found – check connections.")
        return

    with dai.Device(pipeline, devs[0], dai.UsbSpeed.SUPER) as device:
        qRgb   = device.getOutputQueue(name="rgb",   maxSize=4, blocking=False)
        qDepth = device.getOutputQueue(name="depth", maxSize=4, blocking=False)

        cv2.namedWindow("RGB", cv2.WINDOW_NORMAL)

        frame_count = 0
        while True:
            # Break if window closed
            if cv2.getWindowProperty("RGB", cv2.WND_PROP_VISIBLE) < 1:
                break

            # 6) Get color frame and show it
            inRgb = qRgb.get()
            frame = inRgb.getCvFrame()
            cv2.imshow("RGB", frame)

            # 7) Get depth tensor, reshape
            inDepth     = qDepth.get()
            depth_data  = inDepth.getFirstLayerFp16()
            depth_frame = np.array(depth_data, dtype=np.float32).reshape((480, 640))

            # 8) Print the entire depth matrix (stdout)
            print(f"\n--- Depth Frame #{frame_count} (480×640 floats) ---")
            print(depth_frame)
            frame_count += 1

            # 9) Exit on 'q'
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
