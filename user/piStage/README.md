# EUDAQ2 Producer for the PI Stages
Activate this producer by ```BUILD_USER_PISTAGE=on``` in the ```cmake``` configuration
## Acknowledgement
The producer is based on a control library developed by S. Mersi and  P. Wimberger. The original library code can be found here: ```https://gitlab.cern.ch/mersi/RotaController/```
It is distributed under the MIT license:

Copyright 2018 CERN
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Requirements
To run the librar a funcitonla installation of the ```pi_pi_gcs2``` library provided together with the Stages. The software will be provided on the DESY RC-PCs. If the library is missing cmake should complain

## Components

- ```hardware``` contains the adjusted controller from S. Mersi and P. Wimberger
- ```module``` contains the Producer
- ```MISC``` contains test scripts and exemplary configs.

## Usage
This section needs to be written as soon as everything is debugged and finialized.
