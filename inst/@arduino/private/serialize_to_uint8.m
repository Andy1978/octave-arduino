## cast to type "C" and serialize into uint8 byte sequence
## The elements in a matrix are converted in fortran order.
## always uses big-endian byte order, independently of computer architecture

function ret = serialize_to_uint8 (val, c)

  persistent endian = nthargout (3, @computer);

  val = cast (val, c);

  if (endian == "L")
    val = swapbytes (val);
  endif

  # always return row vector
  ret = reshape(typecast (val, "uint8"), 1, []);

endfunction

%!assert (serialize_to_uint8 (1234, "int16"), uint8([4 210]))
%!assert (serialize_to_uint8 ([1234 45054], "uint16"), uint8([4 210 0xAF 0xFE]))
%!assert (serialize_to_uint8 ([8, 3; -2, 5], "int16"), uint8([0 8 255 254 0 3 0 5]))
