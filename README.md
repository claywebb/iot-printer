# iot-printer
This is the code that powers a daily memo printer.

### Parts
- [Mini Thermal Receipt Printer Starter Kit](https://www.adafruit.com/product/600)
- ESP-8266 NodeMCU development board
- Metal Pushbutton with LED Ring

## Client

You can find the arduino code in the client folder. Change the backend url to your server url and flash the code to your arduino.

## Server

The backend of this project is a flask server. It generates the memo in the format of a Jinja2 templated html page. It then converts this html page into a png image of the correct width for a printer using *wkhtmltoimage*. The image is then converted to a bitmap image and sent to the printer in json format in small chunks.

To run the server, first install **wkhtmltopdf**

Then start the flask server:
```
cd server
export FLASK_ENV=development
export FLASK_APP=app.py
flask run
```
* note: you might have to install some dependencies using **pip install**