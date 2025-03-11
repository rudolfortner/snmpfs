#!/bin/bash

# INSTALL PACKAGES FROM REPOSITORY
PACKAGES="cmake g++ libfuse3-dev libsnmp-dev"
for package in $PACKAGES; do
	if dpkg -l "$package" | grep -q ^ii;
	then
		echo "Package $package already installed..."
	else
		sudo apt-get install -y --no-install-recommends "$package"
	fi
done



# INSTALL FROM OTHER SOURCES

mkdir -p deps
cd deps

# Download TinyXML
wget https://github.com/leethomason/tinyxml2/archive/refs/tags/10.0.0.zip -O tinyxml2.zip
unzip -o tinyxml2.zip

mkdir -p tinyxml2/include
mkdir -p tinyxml2/src

mv tinyxml2-10.0.0/tinyxml2.h	tinyxml2/include
mv tinyxml2-10.0.0/tinyxml2.cpp	tinyxml2/src
rm -rf tinyxml2-10.0.0


