# kwm2-cw
CW support in the Collins KWM-2 was not good.  In order to transmit code, they used an audio oscillator inputting into the SSB transmit chain.  To make this work, they used a high injection frequency - in my radio, 1750Hz.  So the user hears 1750 Hz as the sidetone, and also must listen to their QSO partner at 1750 Hz.  Very high and squeaky.  A comfortable 600hz would have landed its second harmonic inside the bandpass of the mechanical filter.
My card fixes this by translating the 1750Hz ( either sidetone or other signal, doesn't matter ) to 650Hz.  It does so by first bandpass filtering the 1750Hz.  Then mixing it down to 650Hz with an 1100Hz oscillator.  Then a high pass filter to chop off the unwanted image frequency.  The card does this all in *software*.  It has a PJRC Teensy 4.1 processor card, and I used the PJRC audio library to implement it.
This depository includes the KiCAD files, the gerbers, and the Arduino sketch.
The card mounts to the back of the KWM-2 PTO.  There's a couple of convenient studs sticking out of it.
In my implementation, I built a power supply mounted on a tube plug, plugged into the noise blanker socket.
I also made room for a BHI noise reduction module.  So now, when I set my KWM-2 to noise blanker mode, I get DSP noise reduction.  Actual noise blankers don't do much nowadays;  The Woodpecker is long gone, and I don't operate mobile with my boatanchors.
The CW fixer also supports two bandwidths - wide and narrow.  When you select CW from the mode switch, you get one of those bandwidths.  Select it again, and you get the other one.
There is an unused lug on the mode switch that gives a ground in CW mode - very convenient!
