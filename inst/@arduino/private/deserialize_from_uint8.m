## deserialize from uint8 into class "C"
## always assume input in big-endian byte order, independently of computer architecture

function ret = deserialize_from_uint8 (val, c)

  persistent endian = nthargout (3, @computer);

  if (! isa (val, "uint8") || ! isvector (val))
    error ("VAL has to be an uint8 vector")
  endif

  ret = typecast (val, c);

  if (endian == "L")
    ret = swapbytes (ret);
  endif

endfunction

%!assert (deserialize_from_uint8 (uint8([0 8 255 254 0 3 0 5]), "int16"), int16([8 -2 3 5]))

%!function ret = ser_deser (val, c)
%!  byte_seq = serialize_to_uint8 (val, c);
%!  assert (isa (byte_seq, "uint8"));
%!  val_out = _deserialize_from_uint8 (byte_seq, c);
%!  assert (isequal (val_out, val)); # ignore class
%!endfunction

%!test ser_deser (1234, "uint16")
%!test ser_deser (-15213, "int16")
%!test ser_deser (pi, "double")
%!test ser_deser ([5 -3 8], "int32")
