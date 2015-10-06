./lepdump b - | gst-launch-1.0 fdsrc ! queue ! udpsink host=192.168.219.24 port=52157
