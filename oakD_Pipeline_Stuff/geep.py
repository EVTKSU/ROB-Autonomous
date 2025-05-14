import depthai as dai
pipeline = dai.Pipeline()
device = dai.Device(pipeline, dai.DeviceInfo("169.254.1.222")) #IP of camera (not of you)
print("Found devices:", device)