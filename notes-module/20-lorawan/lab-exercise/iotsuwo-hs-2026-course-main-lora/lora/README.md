# LoRa Demo with Arduino Uno and LoRa Dev Board 

## Guidelines for installing, executing LoRa Application on the Arduino Dev Board, and transmiting sensor data to The Things Network (TTN) cloud:

### 1. Arduino Dev Board: 

```bash
1. Open the "ttn-otta.ino" application in the Arduino IDE 
```

```bash
2. Uncomment the source code lines that defines "APPEUI", "DEVEUI" and "APPKEY" 
```

```bash
3. Update the values "AppEUI", "DevEUI" and "AppKEY" accordingly with the corresponding numbers of your LoRa device 
```

### 2. Arduino libraries folder located in the folder: "/home/Arduino": 

```bash
1.  "$ cd Arduino/libraries"  => Go to the "Arduino/libraries" folder in your home directory
```

```bash
2.  Check here that you have the following sub-folders provided on GitLab:
a) "Adafruit_CCS811_library"
b) "Adafruit_Si7021_library"
c) "arduino-lmic-master"
```

### 3. Compile and Install the LoRa App on the LoRa device: 

a) Select the below configurations in the Arduino IDE for developing and running the LoRa App source code on the Arduino Uno Board: 
```bash
Tools => Board => "Arduino/Genuino Uno"
```

```bash
Port => "/dev/ttyACM0"
```

```bash
Serial Monitor => /dev/ttyUSB0 => "9600 baud"
```

```bash
Programmer => "AVRISP mkll"
```

b) All should now compile very well and you are all set on the LoRa device side, with the App code + all the libraries compiled successfully.

c) Flash the "ttn-otta.ino" LoRa application on the Arduino Dev Board by clicking on the "Upload" button.

### 4. The Things Network (TTN) Website:

Create your account and register your LoRa device using the steps described in detail at the following link:
https://www.thethingsindustries.com/docs/

### 5. Arduino IDE - Serial Console:

You should see now in the Arduino IDE "Serial Monitor" console that the sensor data is monitored every 10 seconds and after 5 rounds the average sensors values are sent to the Lora TTN Cloud. You can notice the payload values on the website in your corresponding account.

