import depthai as dai
pipeline = dai.Pipeline()
device = dai.Device(pipeline, dai.DeviceInfo("10.10.10.177"))
print("Found devices:", device)