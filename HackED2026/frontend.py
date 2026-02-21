import socket
import re
import random
from math import sqrt
import time
from typing import Tuple
from scipy.optimize import differential_evolution
import matplotlib.pyplot as plt
import matplotlib.animation as animation


class CommandTransmission():
    # If received messages contain the following strings, don't print the message
    filters = [] # ["AT+RANGE", "Response: OK"]

    def __init__(self):
        # TODO: leave this functionality for sending messages to the boards
        # self.esp_id = {
        #     1: "192.168.86.32",
	    #     2: "192.168.1.33"
        # }
        self.ESP32_PORT = 4210
        self.LOCAL_PORT = 4210  # <-- fixed local port on your PC
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.sock.bind(("", self.LOCAL_PORT))  # Force the socket to be 4210
        

    def send_command(self, id, message):
        ip_address = self.esp_id[id]
        self.sock.sendto(message.encode('utf-8'), (ip_address, self.ESP32_PORT))

        # Echo back messages received
        try:
            data, addr = self.sock.recvfrom(1024)
            print(f"Echo from {addr}: {data.decode()}")
        except socket.timeout:
            print("No echo received.")


    def get_range_data(self):
        # Receives commands from any ip addresses (not just the ones in esp_id)
        try:
            data, addr = self.sock.recvfrom(1024)
            message = data.decode('utf-8', errors='ignore').strip()
            
            for blacklisted_string in self.filters:
                if blacklisted_string in message:
                    return
            
            print(message)
            pattern = r'(-?\d+,\s*){7}-?\d+'
            match = re.search(pattern, message)
            
            if match:
                distances = [int(x.strip()) for x in match.group().split(',')]
                print(distances) 
                return distances               
        
        except socket.timeout:
            print("No echo received.")


class UserInterface:
    
    def __init__(self, fig, ax, base_station):
        self.ax = ax
        self.fig = fig
        self.base_station = base_station
        self.points_to_draw = {}
        

    def update_data(self, points_dict):
        self.points_to_draw = points_dict.copy()


    def animate(self, frame):
        self.base_station.trilateration()
        self.points_to_draw = self.base_station.points.copy()

        self.ax.clear()

        # Dynamically set the plot size
        xs = [x for x, y in self.points_to_draw.values()]
        ys = [y for x, y in self.points_to_draw.values()]
        padding = 10
        min_x, max_x = min(xs) - padding, max(xs) + padding
        min_y, max_y = min(ys) - padding, max(ys) + padding

        # TODO: Set axis limits (could dynamically calculate from points if needed)
        self.ax.set_xlim(min_x, max_x)
        self.ax.set_ylim(min_y, max_y)
        self.ax.grid(True)

        updatedArtists = []

        # Display coordinates on the graph
        for label, (x, y) in self.points_to_draw.items():
            if x is not None and y is not None:
                scatter = self.ax.scatter(x, y, label=label)
                text = self.ax.text(x + 0.1, y + 0.1, label, fontsize=9)
                updatedArtists.extend([scatter,text])
        
        # Place lines between the rescuer/victim and the anchors
        rescuerCoords = self.points_to_draw.get("rescuer", None)
        if rescuerCoords is not None:
            rx, ry = rescuerCoords
            for label, (x,y) in self.points_to_draw.items():
                if label != "rescuer" and x is not None and y is not None:
                    if label == 'victim':
                        line = self.ax.plot([x, rx], [y, ry], color='red', linestyle='--')[0]
                    else:
                        line = self.ax.plot([x, rx], [y, ry], color='gray', linestyle='--')[0]
                    updatedArtists.append(line)

        legend = self.ax.legend(loc="upper right")
        updatedArtists.append(legend)

        return updatedArtists
    

    def start_animation(self):
        self.ani = animation.FuncAnimation(self.fig, self.animate, interval=100, cache_frame_data=False)


class Device():

    def __init__(self, x_coordinate: float, y_coordinate: float):
        self.x_coordinate = x_coordinate
        self.y_coordinate = y_coordinate


    def get_coordinates(self) -> Tuple[float, float]:
        """Returns the ground truth coordinates of the device.

        Returns:
            Tuple[float, float]: the x and y coordinates of the device.
        """
        return self.x_coordinate, self.y_coordinate
    
    
    def set_coordinates(self, new_x: float, new_y: float) -> None:
        """Used to set updated ground truth coordinates for the device. 

        Args:
            new_x (float): The x coordinate.
            new_y (float): The y coordinate.
        """
        self.x_coordinate = new_x
        self.y_coordinate = new_y


class BaseStation():
    """Sets up CommandTransmission, UserInterface, and facilitates getting 
    distances, calculating positions, and plotting coordinates.
    """
     
    def __init__(self):
        # Used to facilitate WiFi transmission between boards and the computer
        self.data_obj = CommandTransmission()
        self.points = {}        
        self.init_static_points()
        self.main()


    def init_static_points(self):
        self.points = {
            "anchor0": anchor0.get_coordinates(),
            "anchor1": anchor1.get_coordinates(),
            "anchor2": anchor2.get_coordinates(),
            "victim": victim.get_coordinates(),
        }


    def display_coordinates(self, points: dict):
        """
        Plots named points on a 2D graph using matplotlib.
        
        Parameters:
            coord_dict (dict): Dictionary with names as keys and (x, y) tuples 
            as values.
        """
        for name, (x, y) in points.items():
            plt.scatter(x, y, label=name)
            plt.text(x, y, name, fontsize=9, ha='right', va='bottom')

        plt.axhline(0, color='black', linewidth=1)  # y=0 line (horizontal)
        plt.axvline(0, color='black', linewidth=1)  # x=0 line (vertical)

        plt.xlabel('X')
        plt.ylabel('Y')
        plt.title('Named Coordinates')
        plt.grid(True)
        plt.legend()
        plt.axis('equal')  # optional: equal scaling for x and y
        plt.show()


    def calculate_coordinates(self, calibration=False, trilateration=False):
        if calibration:
            self.calibration()

        if trilateration:
            self.trilateration()


    # TODO: Need to change this function to bridge sensor data
    def get_distance(device1: Device, device2: Device) -> float:
        x = 0
        y = 1

        x_delta = device1.get_coordinates[x] - device2.get_coordinates[x]
        y_delta = device1.get_coordinates[y] - device2.get_coordinates[y]
        magnitude = sqrt(x_delta**2 + y_delta**2)
        
        return magnitude


    def calibration(self) -> None:
        """Uses the relative distances between devices to calculate relative
        coordinates for each device.
        """

        # Hard code these values to make the equation solvable
        a0x, a0y, a1x = 0, 0, 0
        
        # Retrieve the distances between each device
        # TODO: write a bridging function to extract the distances from each device to each other
        a0a1 = get_distance(anchor0, anchor1)
        a0a2 = get_distance(anchor0, anchor2)
        a0vt = get_distance(anchor0, victim)
        a1a2 = get_distance(anchor1, anchor2)
        a1vt = get_distance(anchor1, victim)
        a2vt = get_distance(anchor2, victim)

        def calibration_callback(unknowns):
            a1y, a2x, a2y, vtx, vty = unknowns
            eq1 = (a1x - a0x)**2 + (a1y - a0y)**2 - a0a1**2 
            eq2 = (a2x - a0x)**2 + (a2y - a0y)**2 - a0a2**2 
            eq3 = (vtx - a0x)**2 + (vty - a0y)**2 - a0vt**2  
            eq4 = (a2x - a1x)**2 + (a2y - a1y)**2 - a1a2**2
            eq5 = (vtx - a1x)**2 + (vty - a1y)**2 - a1vt**2
            eq6 = (vtx - a2x)**2 + (vty - a2y)**2 - a2vt**2
            return eq1**2 + eq2**2 + eq3**2 + eq4**2 + eq5**2 + eq6**2

        # Define bounds for each variable
        bounds = [(-100, 100)] * 5

        # Perform global optimization using differential evolution
        result = differential_evolution(calibration_callback, bounds)

        # Output result
        if result.success:
            a1y, a2x, a2y, vtx, vty = result.x
            
            # Update each devices calculated coordinates
            anchor0.set_calc_coordinates(a0x, a0y)
            anchor1.set_calc_coordinates(a1x, a1y)
            anchor2.set_calc_coordinates(a2x, a2y)
            victim.set_calc_coordinates(vtx, vty)

            # Update the internal points object
            self.points = {
                "anchor0": (a0x, a0y),
                "anchor1": (a1x, a1y),
                "anchor2": (a2x, a2y),
                "victim": (vtx, vty)
            }
            
            print("Solution found:")

            for device in self.points:
                print(f"{device}: \t({self.points[device][0]:.3f}, {self.points[device][1]:.3f})")
        else:
            print("No solution found.")


    def trilateration(self):
        
        # TODO: Need to add functionality to accommodate the distances from 
        #TODO: either a victim or rescuer
        distances = self.data_obj.get_range_data()
        if not distances:
            return
        
        a0, a1, a2 = distances[:3]

        a0x, a0y = anchor0.get_coordinates()
        a1x, a1y = anchor1.get_coordinates()
        a2x, a2y = anchor2.get_coordinates()

        def trilateration_callback(unknowns):
            rtx, rty = unknowns
            eq1 = (a0x - rtx)**2 + (a0y - rty)**2 - a0**2 
            eq2 = (a1x - rtx)**2 + (a1y - rty)**2 - a1**2 
            eq3 = (a2x - rtx)**2 + (a2y - rty)**2 - a2**2  
            return abs(eq1) + abs(eq2) + abs(eq3)

        # Define bounds for each variable
        bounds = [(-1000, 1000)] * 2

        # Perform global optimization using differential evolution
        result = differential_evolution(trilateration_callback, bounds)

        # Output result
        if result.success:
            rtx, rty = result.x

            rescuer_tag.set_coordinates(rtx, rty)

            # Update the internal points object
            self.points["rescuer"] = (rtx, rty)
            
            print("Solution found:")

            for device in self.points:
                print(f"{device}: \t({self.points[device][0]:.3f}, {self.points[device][1]:.3f})")
        
            self.visual_obj.update_data(self.points)
        
        else:
            print("No solution found.")


    def main(self):
        # Establish the plot objects
        fig, ax = plt.subplots()
        self.visual_obj = UserInterface(fig, ax, self)

        # Determine where each static device is (the anchors and victim)
        # self.calculate_coordinates(calibration=True, trilateration=False)

        self.visual_obj.update_data(self.points)
        self.visual_obj.start_animation()

        plt.show()


if __name__=="__main__":

    # For now, hard code these locations
    anchor0 = Device(205, 325)
    anchor1 = Device(0, 0)
    anchor2 = Device(205, 0)
    # These two tags need to be initialized
    victim = Device(0, 0)
    rescuer_tag = Device(0, 0)

    # Run the program
    obj = BaseStation()


"""
- Get victim working. 
- Embed delay into set role
- Get rotation working
- Get calibration working


0  10.42.0.0    anchor0  
1  10.42.0.10   anchor1
2  10.42.0.20   anchor2
3  10.42.0.30   victim (anchor)
4  10.42.0.40   rescuer (tag)
"""