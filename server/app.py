#!env/bin/python3
"""Flask server for iot-printer."""

import os
import time
from urllib.request import urlopen
from shutil import copyfileobj
from datetime import date
from flask import Flask, jsonify, request, render_template
from PIL import Image
from bs4 import BeautifulSoup as soup
import yfinance as yf
from functions import convert_range_to_bitmap

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

app = Flask(__name__)

def get_context():
    """Retrive the context for rendering the memo template."""
    context = dict()

    today = date.today()
    context["date"] = today.strftime("%B %d, %Y")
    context["city"] = "Ann Arbor, MI"

    context["stocks"] = []
    stock_symbols = ["^DJI", "SPY"]
    for symbol in stock_symbols:
        price = dict()
        stock = yf.Ticker(symbol)
        price["symbol"] = symbol
        price["price"] = "{:.2f}".format(stock.info["regularMarketPrice"])

        context["stocks"].append(price)

    news_url = "https://news.google.com/news/rss"
    client = urlopen(news_url)
    xml_page = client.read()
    client.close()
    soup_page = soup(xml_page, "lxml")
    news_list = soup_page.findAll("item")

    context["news"] = []
    for i in range(0, min(3, len(news_list))):
        news = news_list[i]
        context["news"].append(news.title.text)

    return context

@app.route('/html')
def html():
    """Display the memo as a html website inside of json.

    For debugging purposes.
    """
    context = get_context()
    return render_template("memo.html", **context)

@app.route('/memo')
def memo():
    """Return the memo as json for the iot printer."""
    pos = int(request.args.get("pos"))
    length = int(request.args.get("length"))
    filename = request.args.get("ts")

    if filename is None or filename == "0":
        context = get_context()

        strio = StringIO()
        strio.write(render_template("memo.html", **context))

        filename = str(int(time.time()))
        with open("tmp/"+filename+".html", 'w') as file:
            strio.seek(0)
            copyfileobj(strio, file)

        os.system("wkhtmltoimage --width 384 --disable-smart-width " +
                  "tmp/" + filename + ".html tmp/" + filename + ".png")

    img = Image.open('tmp/' + filename + '.png')
    width, height = img.size
    px_data = list(img.getdata())

    bw_data = convert_range_to_bitmap(px_data, pos, length)

    response = dict()
    response["hasMore"] = 1 if (pos + length) * 8 < width*height else 0
    response["data"] = bw_data
    response["ts"] = int(filename)

    return jsonify(response)


if __name__ == '__main__':
    app.run(host='192.168.1.15', debug=True)
