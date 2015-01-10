# Creates everything
all: mandelbrot mandelDisplay mandelCalc

#Create mandelbrot executable
mandelbrot: mandelbrot.cpp
	g++ mandelbrot.cpp -o mandelbrot

#Create mandelDisplay executable
mandelDisplay: mandelDisplay.cpp
	g++ mandelDisplay.cpp -o mandelDisplay

#Create mandelDraw executable
mandelCalc: mandelCalc.cpp
	g++ mandelCalc.cpp -o mandelCalc

clean:
	rm -rf mandelbrot mandelDisplay mandelCalc
