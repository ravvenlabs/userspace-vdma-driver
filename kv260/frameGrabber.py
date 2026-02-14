# Dr. Kaputa
# Frame Grabber

import os
import cv2
import numpy as np
import time
import mmap
import struct
import sys, random
import ctypes
import copy

class ImageFeedthrough(object):
  def __init__(self,width,height,depth):
    self.lib = ctypes.cdll.LoadLibrary('/home/kria/imageFeedthroughDriver.so')
    result = self.lib.init(b"vdma1-read-reg",b"vdma1-read-buf1",b"vdma1-read-buf2",b"vdma1-read-buf3",width,height,depth)
    self.receiveFrame= np.ones((height,width,depth), dtype=np.uint8)
    
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
  def __init__(self,width,height,depth):
    #self.lib = ctypes.cdll.LoadLibrary('/home/kria/imageProcessingDriver.so')
    #result = self.lib.init(b"vdma2-read-reg",b"vdma2-read-buf1",b"vdma2-read-buf2",b"vdma2-read-buf3",752,480,8)
    #self.receiveFrame= np.ones((480,752,8), dtype=np.uint8)
    self.lib = ctypes.cdll.LoadLibrary('/home/kria/imageFeedthroughDriver.so')
    result = self.lib.init(b"vdma2-read-reg",b"vdma2-read-buf1",b"vdma2-read-buf2",b"vdma2-read-buf3",width,height,depth)
    self.receiveFrame= np.ones((height,width,depth), dtype=np.uint8)

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
  def __init__(self,width,height,depth):
    self.lib = ctypes.cdll.LoadLibrary('/home/kria/imageWriterDriver.so')
    #result = self.lib.init()
    result = self.lib.init(b"vdma3-write-reg",b"vdma3-write-buf",width,height,depth)
    self.f2 = open("/dev/mem", "r+b")
    self.switchMem = mmap.mmap(self.f2.fileno(), 1000, offset=0xa0050000)
    
  def setFrame(self,frame):
    result = self.lib.setFrame(ctypes.c_void_p(frame.ctypes.data))
    mv = memoryview(self.switchMem).cast('Q') 
    mv[0] = 1
    #self.switchMem.seek(0) 
    #self.switchMem.write(struct.pack('l', 1))

  def __del__(self):
    result = self.lib.destroy