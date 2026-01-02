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

camFeedthrough = ImageFeedthrough()
camProcessing = ImageProcessing()
camWriter = ImageWriter()

time.sleep(1)
print("writing frame to FPGA")
data = np.zeros((480,752,8), dtype=np.uint8)
data[:,200,:] = 250 
data[:,552,:] = 250 
data[200,:,:] = 250 
data[280,:,:] = 250 
#data[:,:,:] = 52
camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)
time.sleep(1)
camWriter.setFrame(data)

frameLeft,frameRight = camFeedthrough.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camFeedthrough.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camFeedthrough.getStereoRGB()    
cv2.imwrite("leftFeedthrough.jpg", frameLeft)
cv2.imwrite("rightFeedthrough.jpg", frameRight)

frameLeft,frameRight = camProcessing.getStereoRGB()    
time.sleep(1)
frameLeft,frameRight = camProcessing.getStereoRGB()   
time.sleep(1) 
frameLeft,frameRight = camProcessing.getStereoRGB()  
time.sleep(1)  
cv2.imwrite("leftProcessing.jpg", frameLeft)
cv2.imwrite("rightProcessing.jpg", frameRight)