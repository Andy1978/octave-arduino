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
## @deftypefn {} {@var{retval} =} __sendCommand__ (@var{obj}, @var{cmd}, @var{data}, @var{timeout})
## Private function
## @end deftypefn

## Author: jdonoghue <jdonoghue@JYRW4S1>
## Created: 2018-05-15

function [dataOut, errcode] = __sendCommand__ (obj, libid, cmd, data = [], timeout = 0.5)
   if nargin < 3
     error ("@arduino.__sendCommand__: expected command");
   endif

   % send command and get back reponse
   if !isa(obj.connected, "octave_serialport") && !isa(obj.connected, "octave_tcp")
     error ("@arduino.__sendCommand__: not connected to a arduino");
   endif

   % sends A5 EXT CMD datasize [data,,,]
   % currently ext is 0 - may use later to identify module to send to ?
   % A5 00 00 00 = reset
   % A5 00 01 00 = req board info
   dataOut = [];
   errcode = 0;

   if iscell(data)
     data = cell2mat(data);
   endif

   set(obj.connected, "timeout", timeout);

   send_buf = uint8([0xA5 libid cmd serialize_to_uint8(numel(data),"uint16") data]);

   len = fwrite(obj.connected, send_buf);

   if (obj.debug)
     printf(">> "); printf("%d ", send_buf); printf("\n");
   endif

   [dataOut, errcode] = __recvResponse__ (obj.connected, libid, cmd, timeout, obj.debug);
endfunction
