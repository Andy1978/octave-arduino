## Copyright (C) 2019 John Donoghue <john.donoghue@ieee.org>
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
## @deftypefn {} {@var{dataOut} =} readWrite (@var{spi}, @var{dataIn})
## Write uint8 data to spi device and return
## back clocked out response data of same size.
##
## @subsubheading Inputs
## @var{spi} - connected spi device on arduino
##
## @var{dataIn} - uint8 sized data to send to spi device framed between SS frame.
##
## @subsubheading Outputs
## @var{dataOut} - uint8 data clocked out during send to dataIn.
##
## @seealso{arduino, device}
## @end deftypefn

function dataOut = writeRead (this, dataIn, first_block_len = 0, delay_us = 0)
  dataOut = [];

  persistent ARDUINO_SPI_READ_WRITE = 2;

  if nargin < 2
    error ("@device.writeRead: expected dataIn");
  endif

  if !strcmp(this.interface, "SPI")
    error("@device.writeRead: not a SPI device");
  endif

  if (first_block_len > 255)
    warning ("@device.writeRead: first_block_len clamped to 255");
  endif

  if (delay_us > 255)
    warning ("@device.writeRead: delay_us clamped to 255 us");
  endif

  out_buf = uint8([this.id delay_us first_block_len dataIn]);
  [dataOut, sz] = sendCommand (this.parent, this.resourceowner, ARDUINO_SPI_READ_WRITE, out_buf);
endfunction

%!shared arduinos
%! arduinos = scanForArduinos(1);

%!assert(numel(arduinos), 1);

%!test
%! ar = arduino();
%! assert(!isempty(ar));
%! spi = device(ar, "SPIChipSelectPin", "d10");
%! assert(!isempty(spi));
%! data = writeRead(spi, 1);
%! assert(numel(data), 1);
%! delete(spi)
%! delete(ar)

%!test
%! ar = arduino();
%! assert(!isempty(ar));
%! spi = device(ar, "SPIChipSelectPin", "d10");
%! assert(!isempty(spi));
%! data = writeRead(spi, [1 1 1]);
%! assert(numel(data), 3);
%! delete(spi)
%! delete(ar)

