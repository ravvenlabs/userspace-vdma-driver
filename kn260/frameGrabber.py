# Dr. Kaputa
# Frame Grabber

import cv2
import numpy as np
import time
import mmap
import struct
import sys, random
import ctypes
import copy

class ImageFeedthrough(object):
  def __init__(self):
    self.lib = ctypes.cdll.LoadLibrary('/fusion2/imageFeedthroughDriver.so')
    result = self.lib.init("vdma1-read-regs","vdma1-read-bufs",752,480,8)
    self.receiveFrame= np.ones((480,752,8), dtype=np.uint8)
    
  def getStereoRGB(self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,0:3],self.receiveFrame[:,:,4:7]

  def getStereoGray (self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,3],self.receiveFrame[:,:,7]
    
  def getStereoAll (self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,0:4],self.receiveFrame[:,:,4:8]
 
  def __del__(self):
    result = self.lib.destroy
        
class ImageProcessing(object):
  def __init__(self):
    self.lib = ctypes.cdll.LoadLibrary('/fusion2/imageProcessingDriver.so')
    result = self.lib.init()
    self.receiveFrame= np.ones((480,752,8), dtype=np.uint8)

  def getStereoRGB(self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,0:3],self.receiveFrame[:,:,4:7]

  def getStereoGray (self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,3],self.receiveFrame[:,:,7]
    
  def getStereoAll (self):
    result = self.lib.getFrame(ctypes.c_void_p(self.receiveFrame.ctypes.data))
    return self.receiveFrame[:,:,0:4],self.receiveFrame[:,:,4:8]
      
  def __del__(self):
    result = self.lib.destroy
    
class ImageWriter(object):
  def __init__(self):
    self.lib = ctypes.cdll.LoadLibrary('/fusion2/imageWriterDriver.so')
    result = self.lib.init()
    self.f2 = open("/dev/mem", "r+b")
    self.switchMem = mmap.mmap(self.f2.fileno(), 1000, offset=0x43c20000)
    
  def setFrame(self,frame):
    result = self.lib.setFrame(ctypes.c_void_p(frame.ctypes.data))
    self.switchMem.seek(0) 
    self.switchMem.write(struct.pack('l', 1))

  def __del__(self):
    result = self.lib.destroy