# How to create esptool executables
It uses [PyInstaller](https://pyinstaller.readthedocs.io/).
Install with pip install pyinstaller
Execute in /esp/esp-idf/components/esptool_py/esptool
pyinstaller --onefile esptool.py

Please note the [esptool license](https://github.com/espressif/esptool)

