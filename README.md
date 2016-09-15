# Linux timex global trainer gps 
This repository contains timex global trainer gps client for linux (timex shame on you!) which allow you at least download all trainings, save them as .tcx and optionaly upload to endomondo. 

Reverse engineered information:
* list of trainings with start time
* list of laps in training with duration and end time
* list of samples/points with: 
	* heart rate
	* heart monitor status 
	* gps latitude, longitude, altitude
	* gps receiver status
	* compass
	* time difference from last sample/point
	* distance difference from last sample/point
	* speed 

System requirements:
* Linux distro (I use ubuntu 14.04 with kernel 3.13.0-68-generic)
* python 2.x (tested with 2.7.6)
* pip (tested with 1.5.4)
* gcc (tested with 4.8.4)

## How to install
Clone this repository
```
git clone https://github.com/jakubsta/timex-linux-trainer-gps
```
Change directory to:
```
cd timex-linux-trainer-gps/timex_lib
```
Execute following commands:
```
python setup.py build
python setup.py install #sudo could be necessary
```
Now you can go to reader_py and install python requirements
```
cd ../reader_py
pip install -r requirements.txt
```
### Not necessary part
To easily find timex device under /dev/ I use udev mechanism with rule from udev directory. To add this rule in your system first you have to find major and minor numbers of your timex watch. To do this you can simply use following command:
```
lsusb | grep "STM32F407" 
```
Response in my case is:
```
Bus 003 Device 013: ID 0483:5740 STMicroelectronics STM32F407
```
So major and minor number of my watch are **0483:5740**.

Next you have to edit udev/10-timex.rules and provide your numbers like in my case **ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740"**. 

Finally add this rule into rules.d
```
sudo cp 10-timex.rules /etc/udev/rules.d/
```

## Usage
To download and save all trainings from watch execute script in reader_py directory
```
./reader.py
```
Your trainings should be saved in exports directory.


## Development
First of all feel free to fork and pull-request.

timex_lib directory contains code which main responsibility is communicate with watch and low level response 'parsing'.

Python's scripts in reader_py are for calling above c library and easily convert response (python's  dicts received from timex_wrapper.c) to .tcx format and optionaly upload to endomondo using [sports-tracker-liberator](https://github.com/isoteemu/sports-tracker-liberator).

### ToDo
* Make configuraction file
* Improve this README file
* Refactor
* Test on traings with pauses
* Traning's sport detection
* Multisport mode support



## Stuff used to make this

 * [sports-tracker-liberator](https://github.com/isoteemu/sports-tracker-liberator) for upload to Endomonfo
 * [global-trainer-gps](https://github.com/talisein/global-trainer-gps) first (I guess) try to decode timex for linux