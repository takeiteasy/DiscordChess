#!/usr/bin/env ruby
require 'socket'
require 'timeout'

sock = nil
begin
  Timeout.timeout 5 do
    sock = TCPSocket.new "127.0.0.1", 8950
  end
rescue SocketError => e
  abort "ERROR: #{e}"
rescue Timeout::Error
  abort "ERROR: Socket connection timed out!"
end

begin
  sock.write "rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR"
  buf = sock.readpartial(46).delete("\u0000")
  if File.exists? buf
    `open #{buf}`
  end
  puts buf
rescue EOFError, SocketError => e
  abort "ERROR: #{e}"
end

#   0 1 2 3 4 5 6 7 
# 8 r n b q k b n r 0
# 7 p p p p p p p p 1
# 6 . . . . . . . . 2
# 5 . . . . . . . . 3
# 4 . . . . . . . . 4
# 3 . . . . . . . . 5
# 2 P P P P P P P P 6
# 1 R N B Q K B N R 7
#   A B C D E F G H 
