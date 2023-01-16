import memcache
import numpy as np
from math import sin,cos,pi
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from random import random as r

Empty=[0,0,0]
server = memcache.Client(['127.0.0.1:11211'], debug=0)
fig = plt.figure()
ax = plt.axes(projection='3d')

ax.set_xlim3d([-2.0, 2.0])
ax.set_xlabel('X')

ax.set_ylim3d([-2.0, 2.0])
ax.set_ylabel('Y')

ax.set_zlim3d([-2.0, 2.0])
ax.set_zlabel('Z')

ax.set_title('Hand Projection')

def finger_pos(finger_arr,angle):
    angle=np.array(angle)*pi/180
    itr=0
    seclen=[]
    jointangle=[]
    while itr<len(finger_arr)-1:
        seclen.append(finger_arr[itr+1][1]-finger_arr[itr][1])
        jointangle.append(0)
        itr=itr+1
    itr=0
    while itr<len(finger_arr)-1:
        for ik in range(itr+1):
            jointangle[itr]=angle+jointangle[itr] 
            '''angle[ik]+jointangle[itr]'''
        finger_arr[itr+1][1]=finger_arr[itr][1]+seclen[itr]*cos(jointangle[itr])
        finger_arr[itr+1][2]=finger_arr[itr][2]-seclen[itr]*sin(jointangle[itr])
        itr=itr+1
    return finger_arr


def update_plot(i,hand_rot,finger_rot):
    resp=server.get('output')
    input=resp.decode('utf-8').split(",")
    hand_rot[0]=float(input[0])
    hand_rot[2]=-float(input[1])
    hand_rot[1]=float(input[2])
    for i in range(5):
        finger_rot[i]=float(input[i+3])
    print(finger_rot)
    [yaw,pitch,roll]=np.array(hand_rot)*pi/180
    [f2_rot,f1_rot,f3_rot,f4_rot,th_rot]=finger_rot
    thumb=np.array([[-0.25,0.63,0],[-0.42,1,0],[-0.5,1.18,0]])
    finger1=np.array([[-0.25,1,0],[-0.25,1.38,0],[-0.25,1.57,0],[-0.25,1.75,0]])
    finger2=np.array([[0,1,0],[0,1.38,0],[0,1.57,0],[0,1.75,0]])
    finger3=np.array([[0.25,1,0],[0.25,1.38,0],[0.25,1.57,0],[0.25,1.75,0]])
    finger4=np.array([[0.5,1,0],[0.5,1.38,0],[0.5,1.57,0],[0.5,1.75,0]])
    hand=np.array([[-0.25,1,0],[-0.25,0,0],[0.25,0,0],[0.5,1,0]])
    thumb=finger_pos(thumb,th_rot)
    finger1=finger_pos(finger1,f1_rot)
    finger2=finger_pos(finger2,f2_rot)
    finger3=finger_pos(finger3,f3_rot)
    finger4=finger_pos(finger4,f4_rot)
    r_arr=np.array([[cos(-roll)*cos(yaw),sin(pitch)*sin(-roll)*cos(yaw)-cos(pitch)*sin(yaw),cos(pitch)*sin(-roll)*cos(yaw)+sin(pitch)*sin(yaw)],[cos(-roll)*sin(yaw),sin(pitch)*sin(-roll)*sin(yaw)+cos(pitch)*cos(yaw),cos(pitch)*sin(-roll)*sin(yaw)-sin(pitch)*cos(yaw)],[-sin(-roll),sin(pitch)*cos(-roll),cos(pitch)*cos(-roll)]])

    hand=np.matmul(r_arr,np.transpose(hand))
    thumb=np.matmul(r_arr,np.transpose(thumb))
    finger1=np.matmul(r_arr,np.transpose(finger1))
    finger2=np.matmul(r_arr,np.transpose(finger2))
    finger3=np.matmul(r_arr,np.transpose(finger3))
    finger4=np.matmul(r_arr,np.transpose(finger4))

    hand_x=np.append(hand[0],hand[0][0])
    hand_y=np.append(hand[1],hand[1][0])
    hand_z=np.append(hand[2],hand[2][0])
   
    hand=ax.plot3D(hand_x,hand_y,hand_z,"gray",linewidth=5)
    thumb=ax.plot3D(thumb[0],thumb[1],thumb[2],"purple",linewidth=5)
    finger1=ax.plot3D(finger1[0],finger1[1],finger1[2],"red",linewidth=5)
    finger2=ax.plot3D(finger2[0],finger2[1],finger2[2],"yellow",linewidth=5)
    finger3=ax.plot3D(finger3[0],finger3[1],finger3[2],"green",linewidth=5)
    finger4=ax.plot3D(finger4[0],finger4[1],finger4[2],"blue",linewidth=5)
    return thumb+finger1+finger2+finger3+finger4+hand

ani = animation.FuncAnimation(fig, update_plot, fargs=(Empty,[Empty,Empty,Empty,Empty,Empty]), interval=25,blit=True)
plt.show()

