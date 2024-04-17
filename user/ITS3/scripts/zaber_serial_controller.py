#!/usr/bin/python3

import argparse
import sys
import serial
from time import sleep
from re import findall


step_size = 0.047625  # [um], as stated in the manual

def arguments():
    parser=argparse.ArgumentParser(description='Zaber moving stage controller')
    parser.add_argument("--move",   action='store_true',  help="move the stages")
    parser.add_argument("--home",   action='store_true',  help="set the home after the stage is powered on")
    parser.add_argument("--getpos", action='store_true',  help="get the position of all the stages (um)")
    parser.add_argument('--movetype', '-m',   type=str,   help='abs or rel movement w.r.t. home', default="rel")   
    parser.add_argument('--step1',            type=float, help='motion step (in um)',             default=0)
    parser.add_argument('--step2',            type=float, help='motion step (in um)',             default=0)
    parser.add_argument('--step3',            type=float, help='motion step (in um)',             default=0)
    parser.add_argument('--step4',            type=float, help='motion step (in um)',             default=0)
    parser.add_argument('--step5',            type=float, help='motion step (in um)',             default=0)
    parser.add_argument('--step6',            type=float, help='motion step (in um)',             default=0)                
    parser.add_argument('--device'  , '-d',   type=str,   help='USB device',                      default="/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AC01ZP4D-if00-port0")
    parser.add_argument('--baudrate', '-r',   type=int,   help='baudrate',                        default=115200)
    args=parser.parse_args()
    return args

def send_serial_command(command, device, baudrate, print_response=False):
    # configure the serial connections (the parameters differ per the device you are connecting to)
    ser = serial.Serial(
        port=device,
        baudrate=baudrate,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=0
    )
    
    ser.isOpen()
    command_to_send = (command + '\r').encode('utf-8')
    ser.write(command_to_send)

    decoded = ''
    if print_response:
        # let's wait two seconds before reading output (lets give device time to answer)
        sleep(.2)
        
        returns = []
        while ser.in_waiting:
            for line in ser.read(1):
                returns.append(chr(line))
                #print(str(count) + str(': ') + chr(line) )

        for i in returns:
            if i != "\x1b":
                # remove the endline chars
                decoded += i
        
    # and close the port again
    ser.close()
    return decoded

def sethome(device, baudrate):
    command = "/home"
    cmd = send_serial_command(command, device, baudrate, print_response=True)

def poweron(device, baudrate):
    # set home
    sethome(device, baudrate)
    # send a command to awake all the moving stages
    command = "/move abs 1000"
    send_serial_command(command, device, baudrate, print_response=True)
    sleep(2)
    # go back home
    sethome(device, baudrate)
    sleep(2)

def getpos(device, baudrate):
    command = "/get pos"
    cmd = send_serial_command(command, device, baudrate, print_response=True)
    # print the position
    regex = "\@\d(\d)\s\d\s(\w+)\s\w+\s\S+\s(\d+)"
    pos_array = cmd.strip().split("\n")
    for res in pos_array:
        ax_pos = findall(regex, res)
        print("axis {} position: {} [um]".format(str(ax_pos[0][0]), str(float(ax_pos[0][2])*step_size)))

def move(movetype, step1, step2, step3, step4, step5, step6, device, baudrate):
    # first plane
    command1 = "/1 move {} {}".format(movetype, int(step1/step_size))
    send_serial_command(command1, device, baudrate, print_response=True)
    command2 = "/2 move {} {}".format(movetype, int(step2/step_size))
    send_serial_command(command2, device, baudrate, print_response=True)

    # second plane
    command3 = "/3 move {} {}".format(movetype, int(step3/step_size))
    send_serial_command(command3, device, baudrate, print_response=True)
    command4 = "/4 move {} {}".format(movetype, int(step4/step_size))
    send_serial_command(command4, device, baudrate, print_response=True)

    # third plane
    command5 = "/5 move {} {}".format(movetype, int(step5/step_size))
    send_serial_command(command5, device, baudrate, print_response=True)
    command6 = "/6 move {} {}".format(movetype, int(step6/step_size))
    send_serial_command(command6, device, baudrate, print_response=True)

    # wait 2 secs, then print the position
    sleep(2)
    getpos(device, baudrate)

if __name__ == "__main__":
    args = arguments()
    if args.move & (args.movetype not in ("abs", "rel")):
        print("Choose between 'abs' or 'rel' movetype")
        sys.exit()
    if args.home:
        poweron(args.device, args.baudrate)
    if args.getpos:
        getpos(args.device, args.baudrate)
    if args.move:
        move(args.movetype, args.step1, args.step2, args.step3, args.step4, args.step5, args.step6, args.device, args.baudrate)

