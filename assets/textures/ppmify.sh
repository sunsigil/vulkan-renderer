#!/usr/bin/env bash
mogrify -format ppm -compress none $(dirname "$0")/*.png
trash $(dirname "$0")/*.png
