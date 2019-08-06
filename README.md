# Air Quality check

## What can we measure

* pm1 / pm2.5 / pm10 / temp / humidity
* startup time is important ( see docs for all, different modules shall wait different time)

## How?

### Different places 

* best place in Waw: GUS - measurements each hour ( [link](http://powietrze.gios.gov.pl/pjp/current/station_details/chart/530) )
    - most dirty, biggest changes during day =+=
    - maybe some other place? smoking? candle and box?
* kitchen, our room, my balcony and flat with air purifier (recreation))
    * right now, after some rains, whole building will have somehow similar (very small, neglectable) values 

### Measurement in time

* devices PM7003 and PM5003 log directly - ~ 1 per sec
* reference is manual -> 1/min resolution (GM8803 uses 5 min average anyway)


## dependencies:

* python3
    * jupyter
    * bokeh
    * matplotlib

## Workflow

* check test data from example - just run notebook `pm_plot.ipnyb` as it is
* gather data i.e. `./pms5003 /dev/ttyUSB0` -> will log to `tmestamp.log` file
    * pm5003 returns nearly proper json (need to add `[` and `]`, remove last `,`) and can be nicelly imported to plot
    * pm_plot.ipynb plots data, needs at least this formated json
    `[
        {
            "t":_unix_timestamp_,
            "dt":_time_from_last_measuremet_,
            "atm_pm1": _digit_,
            "atm_pm2_5": _digit_,
            "atm_pm10": _digit_,
        }
    ]`
* install python3 and dependencies
    * start `jupyter-notebook` and run `pm_plot.ipynb`
    * change log file name in first row
    * run notebook on data to get chart
* gathering pictures of reference devices
    * run all rows one after another, last one will have plot in time
    * connect usbcam, change `/dev/video1` to your cam in `./script.sh`
    * run `script.sh` to capture image each minute
    * run `script2.sh` to make human-as-ocr work
        * this could be automated with python + opencv + tesseract but it would take some time (didn't work nice with my camera)
    * manually rewrite data to json/doc (and export to json) to load to `pm_plot`

------------------------------------------------------------------------------------------------
# From original repository
## pms5003

PMS5003 air quality sensor data grabbing in C, to JSON, on Linux, with optional averaging over time.

Data sheet (kind of…): http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf.

No bullshit approach, does well what needs to be done, and nothing more.

* If you need HTTP, feed its output to curl.

* If you need warmup, use `/bin/sleep`.

* If you need power management, set your GPIO pins in a shell script, for Christ’s sake.

**Beware of Python-based solutions** which constantly eat 100% CPU (unless you `sleep` artificially), and around 100× as much RAM, cf.

* https://github.com/kzyapkov/pms5003-logger/pull/2#issuecomment-354683717,
* https://www.google.com/search?q=python+serial+100+cpu.

### Screenshot

![](screenshot.png)
