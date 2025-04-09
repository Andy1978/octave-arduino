## Copyright (C) 2025 Andreas Weber <andy.weber.aw@gmail.com>
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
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see
## <https://www.gnu.org/licenses/>.

classdef PulseTrain < arduinoio.LibraryBase
  ## -*- texinfo -*-
  ## @deftypefn {} {} arduinoioaddons.PulseTrain
  ## PulseTrain class
  ## @end deftypefn

  properties(Access = public)
    polarity = 1;
		initial_delay = 0;
		pulse_width = 1000; # 1 ms
		pulse_period = 10000; # 10 ms
		cycles = 42;
  endproperties

  properties(GetAccess = public, SetAccess = private)
    pin = [];
  endproperties

  properties(Access = protected, Constant = true)
    # addon properties
    LibraryName = 'PulseTrain/PulseTrain';
    CppHeaderFile = fullfile(arduinoio.FilePath(mfilename('fullpath')), 'src', 'PulseTrainAddon.h');
    CppSourceFile = fullfile(arduinoio.FilePath(mfilename('fullpath')), 'src', 'PulseTrainAddon.cpp');
    CppClassName = 'PulseTrainAddon';
  endproperties

  properties(Access = private)
    # var for scope of cleanupduring release
    cleanup;
  endproperties

  properties(Access = private, Constant = true)
    # command will send to the arduino
    START_COMMAND = 0x01;
    RELEASE_COMMAND = 0x02;
  endproperties

  methods

    function obj = PulseTrain(parentObj, varargin)

      if (!isa (parentObj, "arduino"))
        error("expects arduino object");
      endif

      obj.Parent = parentObj;

      validatePin(obj.Parent, varargin{1}, 'digital')
      obj.pin = getPinInfo(obj.Parent, varargin{1});
			configurePin(obj.Parent, obj.pin.name, "digitaloutput")
			configurePinResource (obj.Parent, obj.pin.name, obj.LibraryName, "digitaloutput", true);

      obj.cleanup = onCleanup (@() sendCommand(obj.Parent, obj.LibraryName, obj.RELEASE_COMMAND, uint8 (obj.pin.terminal)));

    endfunction

    function start(obj)
			data = zeros (1, 16, "uint8");
			data(1) = obj.pin.terminal;
			data(2) = obj.polarity;
			data(3:6)   = typecast (uint32(obj.initial_delay), "uint8");
			data(7:10)  = typecast (uint32(obj.pulse_width), "uint8");
			data(11:14) = typecast (uint32(obj.pulse_period), "uint8");
			data(15:16) = typecast (uint16(obj.cycles), "uint8");

      #obj.Parent.debug = 1;
			[tmp, sz] = sendCommand(obj.Parent, obj.LibraryName, obj.START_COMMAND, data);

    endfunction

    function release(obj)
      obj.cleanup = [];
      # called from cleanup sendCommand(obj.Parent, obj.RELEASE_COMMAND,[obj.Id]);

      # Andy: keine Ahnung, was hier getan wird
      #for i=1:numel(obj.PinInfo)
      #  configurePinResource(obj.Parent, obj.PinInfo{i}.name, obj.PinInfo{i}.owner, obj.PinInfo{i}.mode, true)
      #  configurePin(obj.Parent, obj.PinInfo{i}.name, obj.PinInfo{i}.mode)
      #endfor

    endfunction

    function disp(this)
      printf("    %s with properties\n", class(this));
      printf("          pin.name = %s\n", this.pin.name);
      printf("          polarity = %d\n", this.polarity);
      printf("          initial_delay = %d\n", this.initial_delay);
      printf("          pulse_width = %d\n", this.pulse_width);
      printf("          pulse_period = %d\n", this.pulse_period);
      printf("          cycles = %d\n", this.cycles);
    endfunction

  endmethods

endclassdef
