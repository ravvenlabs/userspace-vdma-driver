# Dr. Kaputa
# performs a loopback test for both feethrough and processing channels
# need to have a .bin file loaded into the FPGA such as the kv260vdma project

# https://github.com/ravvenlabs/vivado-vdma-project

import cv2
import numpy as np
import time
import mmap
import struct
import sys, random
import ctypes
import copy
from frameGrabber import ImageFeedthrough
from frameGrabber import ImageProcessing
from frameGrabber import ImageWriter

width = 1080
height = 720
depth = 8

camProcessed = ImageProcessing(width,height,depth)
camFeedthrough = ImageFeedthrough(width,height,depth)
camWriter = ImageWriter(width,height,depth)

time.sleep(1)
print("writing frame to FPGA")
# generate testing image
data = np.zeros((height,width,depth), dtype=np.uint8)
data[:,200,:] = 250 
data[:,980,:] = 250 
data[100,:,:] = 250 
data[620,:,:] = 250 

camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)
time.sleep(1)

frameLeft,frameRight = camFeedthrough.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camFeedthrough.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camFeedthrough.getStereoRGB()    
cv2.imwrite("leftFeedthrough.jpg", frameLeft)
cv2.imwrite("rightFeedthrough.jpg", frameRight)

camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)
time.sleep(1)

frameLeft,frameRight = camProcessed.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camProcessed.getStereoRGB()   
time.sleep(1) 
frameLeft,frameRight = camProcessed.getStereoRGB()  
time.sleep(1)  
cv2.imwrite("leftProcessed.jpg", frameLeft)
cv2.imwrite("rightProcessed.jpg", frameRight)