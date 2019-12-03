# esp32_cubes Processing Controller

Software based on the Processing Framework for ESP32 based Light device to control RGB lights
via ESP-NOW protocol.

Currently this is used to control the colors of 50 plastic LED Cubes (30x30x30cm)
which are Battery powered. They were retro-fitted with ESP32 Modules.

This Project consists of the following parts:

 * ESP32 as Gateway (../gateway_firmware/)
 * Processing.Org Control Software (this project folder)
 * ESP32 for the Lights (../firmware/) 
 
This project currently runs only without the Processing IDE. We are using IntelliJ to
run it.


![Screenshot of the Software](../doc/processing.png)


## Features

 * Select each cube manually or by modulus,odd,even,random, all or none
 * Set Fadetime and Color to each cube individually
 * Store and Load Presets (Left and Right Arrow Keys)
 * Clear Cube Selection after Color Pick
 * Stores last used colors
 * Shows simple color palette
 * Huge color picker
 * Blackout Button