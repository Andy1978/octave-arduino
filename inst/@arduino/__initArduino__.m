## Copyright (C) 2018-2022 John Donoghue <john.donoghue@ieee.org>
##
## This program is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details. see
## <https://www.gnu.org/licenses/>.

## -*- texinfo -*-
## @deftypefn {} {@var{retval} =} __initArduino__ (@var{obj}, @var{port}, @var{board}, @var{scan_only})
## Private function
## @end deftypefn

function retval = __initArduino__ (obj, port, board, scan_only)

   % send command and get back reponse
   ARDUINO_INIT = 1;
   ARDUINO_GETLIB  = 8;

   ok = false;

   if !isempty(port) || !ischar(port)
     # port maybe ip address ?
     if !isempty(regexp(port, "^[0-9]+.[0-9]+.[0-9]+.[0-9]+$"))
       obj.connected = tcp (port, 9500, 10);
     else
       obj.connected = serialport (port, 'BaudRate', obj.BaudRate, 'Timeout', .2);
     endif
     # need wait for arduino to potentially startup
     pause(2);

     # clear any data in buffers
     set(obj.connected, "timeout", 0.1);
     data = fread(obj.connected,100);
     while length(data) >= 100
       data = fread(obj.connected,100);
       if obj.debug
         printf("flushing %d bytes of data\n", length(data));
       endif
     endwhile

     [dataout, status] = __sendCommand__(obj, 0, ARDUINO_INIT);
     if status != 0
       error ("__initArduino__: failed valid response err=%d - %s", status, char(dataout));
     endif
     % uno r3 - atmega32 1E 95 0F
     sig = (uint32(dataout(1))*256*256) + (uint32(dataout(2))*256) + uint32(dataout(3));
     % work out mcu
     switch sig
	 case 0
	   mcu = "";
         case { hex2dec("1E9502"),  hex2dec("009502") }
	   mcu = "atmega32";
         case { hex2dec("1E950F"),  hex2dec("00950F") }
	   mcu = "atmega328p";
         case { hex2dec("1E9514"),  hex2dec("009514") }
	   mcu = "atmega328pu";
         case hex2dec("1E9801")
	   mcu= "atmega2560";
         case hex2dec("1E9703")
	   mcu = "atmega1280";
         case hex2dec("1E9702")
	   mcu = "atmega128";
         case hex2dec("1E9587")
	   mcu = "atmega32u4";
         case hex2dec("1E9651")
	   mcu = "atmega4809";
	 otherwise
	   mcu = sprintf("unknown_mcu(%X)", sig);
     endswitch

     boardtype = arduinoio.boardTypeString(dataout(4));
     voltref = double(dataout(5))/10.0;
     numlib = uint8(dataout(6));

     flags = 0;
     if length(dataout) > 6
       flags = dataout(7);
     endif

     % check board against config info
     if ~isempty(board) && !strcmpi(board, boardtype)
       warning("connected %s arduino does not match requested board type %s", boardtype, board)
     endif

     obj.config = arduinoio.getBoardConfig(boardtype);
     # update values that could change
     obj.config.port = port;
     obj.config.baudrate = obj.BaudRate;
     obj.config.board = boardtype;
     obj.config.voltref = voltref;
     obj.config.flags = flags;
     if isa(obj.connected, "octave_tcp")
       obj.config.port = 9500;
       obj.config.deviceaddress = port;
     endif
     if ! isempty(mcu)
       obj.config.mcu = mcu;
     elseif isempty(obj.config.mcu)
       obj.config.mcu = "unknown";
     endif
     obj.config.libs = {};

     # query libs
     if ! scan_only
       for libid = 0:numlib-1
         [dataout, status] = __sendCommand__(obj, 0, ARDUINO_GETLIB, [libid]);
         if status != 0
           error ("__initArduino__: failed get lib %d err=%d - %s", libid, status, char(dataout));
         else
	   lib = {};
           lib.id = libid;
	   lib.name = lower(char(dataout(2:end)));
	   obj.config.libs{end+1} = lib;
         endif
       endfor
     endif
   else
     error ("__initArduino__: expected a valid port");
   endif

   retval = obj;

endfunction
