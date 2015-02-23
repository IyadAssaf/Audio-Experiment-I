'use strict';

var tones = require("./build/Release/tones.node")

tones.play(
		    440, // frequency (Hz)
		    0.5, // amplitude (0.0-1.0)
		    2 	 // waveforms:
			 	 //  1: Sine
				 //  2: Triange
				 //  3: Sawtooth
				 //  4: Square
		   );
