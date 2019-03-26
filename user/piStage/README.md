# EUDAQ2 Producer for the PI Stages
Activate this producer by ```USER_PISTAGE_BUILD=on``` in the ```cmake``` configuration
## Acknowledgement
The producer is based on a control library developed by S. Mersi and  P. Wimberger. The original library code can be found here: ```https://gitlab.cern.ch/mersi/RotaController/```
It is distributed under the MIT license:

Copyright 2018 CERN
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Requirements
To run the library a funcitonal installation of the ```pi_pi_gcs2``` library provided together with the Stages. The software will be provided on the DESY RC-PCs. If the library is missing cmake should complain

## Components

- ```hardware``` contains the adjusted controller from S. Mersi and P. Wimberger
- ```module``` contains the Producer
- ```MISC``` contains test scripts and exemplary configs.

## Usage
Exemplary [init](usr/piStage/misc/Rota.ini) and [config](usr/piStage/misc/Rota.conf) snippets are provided. 
The configuration is not able to handle units. Rotations are given in `degree` and axis positions in `mm`.

The producer has to be correctly inititialized with the following parameters. 
- ```ControllerIP``` IP of stage contoller default: `192.168.24.4`
- ```Port``` Port to be used, for all DESY Stages and default: `50000`
- ```axisX``` DC-motor connected to x-axis. No default value. Assumed to be not connected if not set. Typically set to 1 for DESY testbeam.
- ```axisy``` DC-motor connected to y-axis. No default value. Assumed to be not connected if not set. Typically set to 2 for DESY testbeam.
- ```axisRot``` DC-motor connected to rot-axis. No default value. Assumed to be not connected if not set. Typically set to 3 for DESY testbeam.
- ```initX```Init position for x-axis. Default `0`.
- ```initY```Init position for y-axis. Default `0`.
- ```initRot```Init position for rot-axis. Default `0`.
- ```linStageTypeX``` Used stage for x-axis (M-521.DD1 or M-511.DD1).
- ```linStageTypeY``` Used stage for y-axis (M-521.DD1 or M-511.DD1).
- ```rotStageType```  Used stage for rotation (M-060.DG).
The configuration file needs the following up to three elements
- ```positionX```Desired new position for x-axis. If not defined, no movement can be done.
- ```positionY```Desired new position for y-axis. If not defined, no movement can be done.
- ```positionRot```Desired new position for rot-axis. If not defined, no movement can be done.

