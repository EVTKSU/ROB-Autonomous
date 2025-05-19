import os, sys, cv2
import argparse
import depthai as dai


def clear_terminal() -> None:
  """Clears the terminal of previous output
  """
  print('\033c', end = '')
  
  
def parse_args():
  parse = argparse.ArgumentParser(description = "Smoothed Midpoint Line Finder")
  
  parse.add_argument(
    "--ip", "-i", default = os.getenv("OAKD_IP", None),
    help = "OAK‑D‑LR IP for Ethernet (omit for USB)"
  )
  
  parse.add_argument(
    "--white-level", "-w", type = float, default = 30.0,
    help = "White level %% threshold"
  )
  
  return parse.parse_args()


def create_pipeline() -> dai.Pipeline:
  """Creates a pipeline for the Depth AI 

  Returns:
    dai.Pipeline: Pipeline to run the Depth AI on 
  """
  
  pipe = dai.Pipeline()
  cam = pipe.create(dai.node.ColorCamera)
  
  cam.setBoardSocket(dai.CameraBoardSocket.CAM_A) # Sets the camera to the socket CAM_A
  cam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1200_P) # Sets camera resolution
  cam.setInterleaved(False) # Turns of exposure interleaving
  cam.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR) # Sets the camera to Depth AI color scheme
    
  manip = pipe.create(dai.node.ImageManip) # Creates an image maniputation node
  manip.initialConfig.setFrameType(dai.ImgFrame.Type.BGR888p) 
  manip.initialConfig.setResize(640,352) # Resizes the image 
  manip.initialConfig.setKeepAspectRatio(False) # Sets the manipulation node not to keep aspect ratio
  
  cam.video.link(manip.inputImage) # Links the manipulation node to the camera node
  
  xout = pipe.create(dai.node.XLinkOut) # Creates an output stream 
  xout.setStreamName("video") # Sets the output stream name to 'video'
  manip.out.link(xout.input) # Sets the output manipulation to the output stream's input
      
  return pipe


def process_frame(frame, white_pct):
  # 1) Converts the image to grayscale
  gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY) 
  
  # 2) Perform binary thresholding 
  _, thresh = cv2.threshold(gray, 255 * (white_pct / 100), 255, cv2.THRESH_BINARY)
  
  # 3) Find contours in the frame
  contours = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)
  contours = contours[0] if len(contours) == 2 else contours[1]
  
  # 4) Draw countours on the frame
  radius = 2
  color = (0, 255, 255)
  cv2.drawContours(frame, contours, -1, color, radius)
  
  return frame


def main() -> None:
  args = parse_args() # Parses the arguments for the device info
  pipeline = create_pipeline() # Creates a pipeline for the Depth AI
  
  try: # Attempts to make a device given the pipeline
    dev = dai.Device(pipeline, dai.DeviceInfo(args.ip)) if args.ip else dai.Device(pipeline)
  except RuntimeError as e: # Exits if there's a runtime error
    print("Connection failed:", e)
    sys.exit(1)

  q = dev.getOutputQueue("video", maxSize = 4, blocking = False)
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
      w_lvl = cv2.getTrackbarPos("White Level", win)
      out, raw_mid = process_frame(frame, w_lvl)

      if raw_mid is not None:
        if prev_mid is None:
          prev_mid = raw_mid
        else:
          prev_mid = (
            alpha * raw_mid[0] + (1 - alpha) * prev_mid[0],
            alpha * raw_mid[1] + (1 - alpha) * prev_mid[1]
          )
          
        smx, smy = int(prev_mid[0]), int(prev_mid[1])
        
        # Draw a smoothed vertical line and dots
        cv2.line(out, (smx, 0), (smx, out.shape[0]), (0, 0, 255), 2)
        cv2.circle(out, (smx, smy), 6, (0, 0, 255), -1)

      cv2.imshow(win, out)

    if cv2.waitKey(1) & 0xFF == ord('q'):
      break

  cv2.destroyAllWindows()



if __name__ == '__main__':
  clear_terminal()
  main()