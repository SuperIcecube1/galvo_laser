#Import tkinter library
from tkinter import *
#Create an instance of tkinter frame or window
win= Tk()
#Set the geometry of tkinter frame
win.geometry("500x500")
#Disbales resizing
win.resizable(False,False)
def click(location):
   #Converts location to voltage
   voltage = [location.x / 100, location.y / 100]

   print(voltage)

#Detects a click
win.bind('<Motion>', click)
win.mainloop()