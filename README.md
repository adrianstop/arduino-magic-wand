# Arduino Magic Wand
Arduino Magic wand project. The Arduino should run a ML model to recognize five different spells.
The training will happen on Google Colab with TensorFlow and the model will then be exported to TensorFlow Lite to be able to run it on the Arduino.
The Arduino used in this case is the [Arduino Nano 33 BLE Sense](https://docs.arduino.cc/hardware/nano-33-ble-sense), but the C-code should be portable to most Arduinos although the performance can vary.


# Credits
This project is the final project in the course Embedded Machine Learning (448.155) at the Technical University of Graz. 
The course is held by Prof. Olga Saukh and the link to the course GitHub can be found [here](https://github.com/osaukh/embedded_machine_learning).
