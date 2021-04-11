import os
import errno
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


FIFO = 'pipe/mem_pipe'

try:
    os.mkfifo(FIFO)
except OSError as oe: 
    if oe.errno != errno.EEXIST:
        print("Error occured")
z=[]
print("mem opening...")
fifo=open(FIFO)
print("FIFO opened")

fig = plt.figure(figsize=(6,4))
frame_len = 100
plt.ylim(0,100)
plt.xlabel('Time (s)')
plt.ylabel('Memory usage (%)')
#plt.legend(loc = 'upper right')


z=[]            
def animate(i):
    
    data=fifo.read(2)
    
    if "\x00" in data:
        data=data[0]

    a=0
    #print(repr(data))
    try:
    	a=int(data)
    	z.append(a)
    	#print(z)
    except:
    	return

    plt.cla()
    plt.ylim(0,100)
    plt.xlabel('Time (s)')
    plt.ylabel('Memory usage (%)')
    #plt.legend(loc = 'upper right')
    plt.tight_layout()
    
    if len(z) <= frame_len and len(z)>3:
        plt.plot(z, 'r', label='Real-Time Memory usage')
        plt.show()
    elif len(z)>frame_len:
        plt.plot(z[-frame_len:], 'r', label='Real-Time Memory usage')
        plt.show()
    

    
    
    
ani = FuncAnimation(plt.gcf(),animate, interval=700)
plt.tight_layout()
matplotlib.pyplot.show()
