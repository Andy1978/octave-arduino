## Copyright (C) 2025 Andreas Weber <octave@josoansi.de>
##
## This program is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## -*- texinfo -*-
## @deftypefn {} {} writeVoltage (@var{ar}, @var{pin}, @var{voltage})
## Output voltage out of a pin using a on-chip DAC.
##
## @subsubheading Inputs
## @var{ar} - connected arduino object
##
## @var{pin} - pin to write to.
##
## @var{voltage} - voltage between 0 .. board voltage
##
## @subsubheading Example
## @example
## @code{
## a = arduino();
## writeVoltage(a,'D5',1.2);
## }
## @end example
##
## This function is a GNU Octave extension an not (yet) available in Matlab.
##
## @seealso{arduino, writePWMVoltage}
## @end deftypefn

function writeVoltage (obj, pin, value)

  # Currently there is no equivalent in Matlab, see for example
  # https://de.mathworks.com/matlabcentral/answers/2097766-how-to-write-to-dac-pins-on-arduino-in-matlab-not-simulink

  if nargin < 3
    error ("@arduino.writeVoltage: expected pin name and value");
  endif

  # TODO: need look at board type for what voltage range is allowed
  # and convert
  maxvolts = obj.board_voltage();
  if !isnumeric(value) || value < 0 || value > maxvolts
    error('writeVoltage: value must be between 0 and %f', maxvolts);
  endif

	pininfo = obj.get_pin(pin);

  # first use ?
  if strcmp(pininfo.mode, "unset")
    configurePin(obj, pin, "dac")
  else
    [pinstate, pinmode] = pinStateMode(pininfo.mode);
    if !strcmp(pinmode, "dac")
      error ("@arduino.writeVoltage: pin is in incompatable mode");
    endif
  endif

  # TODO: The DAC resolution can be changed with "analogWriteResolution(...)"
  # on the arduino side, see for example
  # https://docs.arduino.cc/tutorials/uno-r4-minima/cheat-sheet/#dac

  # Until now assume 8 bit (default) DAC resolution.
  dac_bits = 8;
  val = value/maxvolts * (2 ^ dac_bits - 1);

	datain = uint8([pininfo.id val]);

	ARDUINO_DAC = 7; # has to match OctaveCoreLibrary.cpp
	[dataout, status] = __sendCommand__ (obj, 0, ARDUINO_DAC, datain);

endfunction

%!shared ar, pwmpin
%! ar = arduino();
%! dacpin = getPinsFromTerminals(ar, getDACTerminals(ar)){1};

%!test
%! writeVoltage(ar, dacpin, ar.AnalogReference);

%!error <undefined> writeVoltage();

%!error <expected> writeVoltage(ar)

%!error <expected pin> writeVoltage(ar, dacpin)

%!error <unknown pin> writeVoltage(ar, "xd1", 1)

%!error <value must be between> writeVoltage(ar, dacpin, -1)

%!error <value must be between> writeVoltage(ar, dacpin, 5.1)

%!test
%! writeVoltage(ar, dacpin, 0.0);
