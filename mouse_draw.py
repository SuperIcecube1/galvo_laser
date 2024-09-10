import tkinter as tk


lastx, lasty = 0, 0
voltages = []




def xy(location):
    "Takes the coordinates of the mouse when you click the mouse"
    global lastx, lasty, laserOn
    lastx, lasty = location.x, location.y
    convertToVoltage(location.x, location.y, False)
 

def addLine(location):
    """Creates a line when you drag the mouse
    from the point where you clicked the mouse to where the mouse is now"""
    global lastx, lasty, voltages,laserOn
    canvas.create_line((lastx, lasty, location.x, location.y))
    # this makes the new starting point of the drawing
    lastx, lasty = location.x, location.y
    convertToVoltage(location.x, location.y, True)
    
def convertToVoltage(x,y,laserOn):
    global voltages
    voltages.append([x / 100, y / 100, laserOn])
    
root = tk.Tk()
root.geometry("500x500")
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)


canvas = tk.Canvas(root)
canvas.grid(column=0, row=0, sticky=(tk.N, tk.W, tk.E, tk.S))
canvas.bind("<Button-1>", xy)
canvas.bind("<B1-Motion>", addLine)

root.mainloop()
print('end')
print(voltages)