import cv2
import numpy as np
import time
import mmap
import struct
import sys, random
import ctypes
import copy
from frameGrabber import ImageProcessing
from frameGrabber import ImageFeedthrough
from frameGrabber import ImageWriter

camProcessed = ImageProcessing()
camFeedthrough = ImageFeedthrough()
camWriter = ImageWriter()

time.sleep(1)
print "writing frame to FPGA"
data = np.zeros((1080,1920,8), dtype=np.uint8)
data[:,100,:] = 200 
data[:,1820,:] = 200 
data[100,:,:] = 200 
data[980,:,:] = 200 
camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)

time.sleep(1)
frameLeft,frameRight = camFeedthrough.getStereoRGB()    
frameLeft,frameRight = camFeedthrough.getStereoRGB()
cv2.imwrite("left.jpg", frameLeft)
cv2.imwrite("right.jpg", frameRight)