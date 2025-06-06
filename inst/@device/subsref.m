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
## @deftypefn {} {@var{val} = } subsref (@var{dev}, @var{sub})
## subref for device
##
## @seealso{device}
## @end deftypefn

function val = subsref (this, s)
  if isempty(s)
    error ("device.subsref missing index");
  endif

  if strcmp(this.interface, "SPI")
    if s(1).type == "."
      fld = tolower(s(1).subs);
      switch (fld)
        case "pins"
	  val = {};
	  for i = 1:numel(this.pins)
	    val{end+1} = this.pins{i}.name;
	  endfor
        case "interface"
	  val = this.interface;
        case "parent"
          val = this.parent;
        case "spimode"
	  val = this.devinfo.mode;
        case "bitrate"
	  val = this.devinfo.bitrate;
        case "bitorder"
          val = this.devinfo.bitorder;
        case "spichipselectpin"
	  val = this.devinfo.chipselectpin;
        case "misopin"
	  val = get_func_pin(this.pins, "miso");
        case "mosipin"
	  val = get_func_pin(this.pins, "mosi");
        case "sckpin"
	  val = get_func_pin(this.pins, "sck");
        otherwise
	  error ("device.subsref invalid property '%s'", fld);
      endswitch
    else
      error("unimplemented device.subsref type");
    endif

  elseif strcmp(this.interface, "Serial")
    if s(1).type == "."
      fld = tolower(s(1).subs);
      switch (fld)
        case "pins"
	  val = {};
	  for i = 1:numel(this.pins)
	    val{end+1} = this.pins{i}.name;
	  endfor
        case "interface"
	  val = this.interface;
        case "parent"
          val = this.parent;
        case "baudrate"
	  val = this.devinfo.baudrate;
        case "databits"
	  val = this.devinfo.databits;
        case "stopbits"
	  val = this.devinfo.stopbits;
        case "parity"
	  val = this.devinfo.parity;
        case "timeout"
	  val = this.devinfo.timeout;
        case "numbytesavailable"
	  val = __getBytesAvailable__(this);
        case "serialport"
	  val = this.devinfo.id;
        otherwise
	  error ("device.subsref invalid property '%s'", fld);
      endswitch
    else
      error("unimplemented device.subsref type");
    endif

  else # I2C
    if s(1).type == "."
      fld = tolower(s(1).subs);
      switch (fld)
        case "pins"
	  val = {};
	  for i = 1:numel(this.pins)
	    val{end+1} = this.pins{i}.name;
	  endfor
        case "interface"
	  val = this.interface;
        case "parent"
	  val = this.parent;
        case "bus"
          val = this.devinfo.bus;
        case "i2caddress"
	  val = this.devinfo.address;
        case "bitrate"
	  val = this.devinfo.bitrate;
        case "sdapin"
	  val = get_func_pin(this.pins, "sda");
        case "sclpin"
	  val = get_func_pin(this.pins, "scl");
        otherwise
	  error ("device.subsref invalid property '%s'", fld);
      endswitch
    else
      error("unimplemented device.subsref type");
    endif
  endif

  if (numel (s) > 1)
    val = subsref (val, s(2:end));
  endif
endfunction

function val = get_func_pin(pins, func)
  val = [];
  for i=1:numel(pins)
    pin = pins{i};
    if strcmp(pin.func, func)
      val = pin.name;
    endif
  endfor
endfunction

%!test
%! ar = arduino();
%! spi = device (ar, "spichipselectpin", "d10");
%! assert (spi.spichipselectpin, "d10")
%! assert (isarduino(spi.parent))
%! assert (ar.port, spi.parent.port)
%! assert (spi.spimode, 0)
%! assert (spi.bitorder, "msbfirst")
%! assert (spi.interface, "SPI")
%! assert (numel(spi.pins) >= 4)
%! fail ("spi.invalid")
%! delete(spi)
